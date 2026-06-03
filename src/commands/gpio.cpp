#include "gpio.h"
#include "OptionParser.h"
#include "lang/LangAPI.h"

#include <chrono>

#if defined(ESP_PLATFORM)
extern "C"
{
#include "esp_timer.h"
}
#endif

#ifndef ET_GPIO_AUTH_TTL_MS
#define ET_GPIO_AUTH_TTL_MS 300000
#endif

using namespace EmbeddedTerminal::cmd;

ETString gpio::usage(const ETString &keyword) const
{
    return keyword + " list - Lists available GPIO pins\n" +
           keyword + " policy - Shows active GPIO policy\n" +
           keyword + " read [pin[,pin...]] - Reads current state of one or more pins\n" +
           keyword + " write [pin[,pin...]] [0|1] - Writes low/high to one or more pins\n" +
           keyword + " mode [pin[,pin...]] [input|output] - Sets mode for one or more pins\n" +
           keyword + " deny [pin[,pin...]] [read|write|mode|include|exclude ...] [--protected] - Denies selected operations\n" +
           keyword + " allow [pin[,pin...]] [read|write|mode|include|exclude ...] - Allows selected operations\n" +
           keyword + " excluded - Lists active exclusion rules\n" +
           keyword + " auth [password|status] - Authenticate or show auth status/remaining time\n";
}

EmbeddedTerminal::CommandResult gpio::invoke(CommandInvocation &invocation)
{
    int exitCode = 0;
    ETString response = run_(invocation.arguments, &invocation.context, exitCode);
    if (!response.empty())
    {
        if (exitCode == 0)
        {
            invocation.streams.output.print(response);
        }
        else
        {
            invocation.streams.error.print(response);
        }
    }
    return CommandResult::completed(exitCode);
}

ETString gpio::run_(const ETVector<ETString> &arguments, CommandContext *context, int &exitCode)
{
    exitCode = 0;
    OptionParser parser;
    parser.addOption("--protected", "--protected", "Mark the policy change as password protected");

    auto parseResult = parser.parse(arguments);
    if (!parseResult.success)
    {
        exitCode = 1;
        return usage("gpio");
    }

    const bool protectedRequested = parseResult.options.find("--protected") != parseResult.options.end() && !parseResult.options["--protected"].empty();
    if (parseResult.remainingArguments.empty())
    {
        return usage("gpio");
    }

    ETString command = parseResult.remainingArguments[0];

    if (command == "list")
    {
        ETString result;
        auto pins = gpio_.listPins();
        if (pins.empty())
        {
            return "No GPIO pins available\n";
        }
        for (const auto &pin : pins)
        {
            ETString resolved = pin.id;
            ETString readReason;
            ETString writeReason;
            ETString modeReason;
            bool canRead = policy_.isOperationAllowed(resolved, GpioOperation::Read, readReason);
            bool canWrite = policy_.isOperationAllowed(resolved, GpioOperation::Write, writeReason);
            bool canMode = policy_.isOperationAllowed(resolved, GpioOperation::Mode, modeReason);
            result += pin.id;
            if (!pin.displayName.empty() && pin.displayName != pin.id)
            {
                result += " (" + pin.displayName + ")";
            }
            result += " [R:" + ETString(canRead ? "Y" : "N") + " W:" + ETString(canWrite ? "Y" : "N") + " M:" + ETString(canMode ? "Y" : "N") + "]\n";
        }
        return result;
    }

    if (command == "policy")
    {
        auto rules = policy_.exclusions();
        ETString result = "Policy: " + policy_.policyName() + "\n";
        result += "Exclusions: " + toETString(rules.size()) + "\n";
        return result;
    }

    if (command == "excluded")
    {
        auto rules = policy_.exclusions();
        if (rules.empty())
        {
            return "No exclusions\n";
        }
        ETString result;
        for (const auto &rule : rules)
        {
            result += formatRule_(rule) + "\n";
        }
        return result;
    }

    if (command == "auth")
    {
        if (auth_ == nullptr)
        {
            exitCode = 1;
            return "Authentication is not configured\n";
        }
        if (context == nullptr)
        {
            exitCode = 1;
            return "Authentication requires interactive command runtime\n";
        }
        if (parseResult.remainingArguments.size() >= 2 && parseResult.remainingArguments[1] == "status")
        {
            if (!isAuthenticated_(context))
            {
                return "Not authenticated\n";
            }

            const uint64_t ttlMs = static_cast<uint64_t>(ET_GPIO_AUTH_TTL_MS);
            if (ttlMs == 0)
            {
                return "Authenticated (no timeout)\n";
            }

            auto expiryIt = context->variables.find(SESSION_KEY_AUTH_UNTIL_MS);
            if (expiryIt == context->variables.end())
            {
                return "Authenticated\n";
            }

            uint64_t expiresAt = static_cast<uint64_t>(ETString::toull(expiryIt->second.c_str()));
            uint64_t now = nowMs_();
            uint64_t remainingMs = (expiresAt > now) ? (expiresAt - now) : 0;
            uint64_t remainingSec = remainingMs / 1000ULL;
            return "Authenticated, expires in " + toETString(static_cast<size_t>(remainingSec)) + "s\n";
        }
        if (parseResult.remainingArguments.size() < 2)
        {
            exitCode = 1;
            return "Usage: gpio auth [password|status]\n";
        }

        if (auth_->verifyPassword(parseResult.remainingArguments[1]))
        {
            context->variables[SESSION_KEY_AUTHENTICATED] = "1";
            const uint64_t ttlMs = static_cast<uint64_t>(ET_GPIO_AUTH_TTL_MS);
            if (ttlMs > 0)
            {
                context->variables[SESSION_KEY_AUTH_UNTIL_MS] = toETString(static_cast<size_t>(nowMs_() + ttlMs));
                return "Authenticated\n";
            }
            context->variables.erase(SESSION_KEY_AUTH_UNTIL_MS);
            return "Authenticated\n";
        }

        context->variables[SESSION_KEY_AUTHENTICATED] = "0";
        exitCode = 1;
        return "Authentication failed\n";
    }

    if (command == "read")
    {
        if (parseResult.remainingArguments.size() < 2)
        {
            exitCode = 1;
            return "Usage: gpio read [pin[,pin...]]\n";
        }
        auto inputPins = parsePins_(parseResult.remainingArguments[1]);
        ETString result;
        for (const auto &pinInput : inputPins)
        {
            ETString resolved;
            ETString error;
            if (!resolveAndAuthorize_(pinInput, GpioOperation::Read, resolved, error))
            {
                exitCode = 1;
                return error + "\n";
            }
            bool value = false;
            if (!gpio_.read(resolved, value))
            {
                exitCode = 2;
                return "Failed to read pin\n";
            }
            result += resolved + "=" + ETString(value ? "1" : "0") + "\n";
        }
        return result;
    }

    if (command == "write")
    {
        if (parseResult.remainingArguments.size() < 3)
        {
            exitCode = 1;
            return "Usage: gpio write [pin[,pin...]] [0|1]\n";
        }

        bool high = (parseResult.remainingArguments[2] == "1" || parseResult.remainingArguments[2] == "high");
        if (!(parseResult.remainingArguments[2] == "0" || parseResult.remainingArguments[2] == "1" || parseResult.remainingArguments[2] == "low" || parseResult.remainingArguments[2] == "high"))
        {
            exitCode = 1;
            return "Invalid level. Use 0/1/low/high\n";
        }

        auto inputPins = parsePins_(parseResult.remainingArguments[1]);
        for (const auto &pinInput : inputPins)
        {
            ETString resolved;
            ETString error;
            if (!resolveAndAuthorize_(pinInput, GpioOperation::Write, resolved, error))
            {
                exitCode = 1;
                return error + "\n";
            }

            if (!gpio_.write(resolved, high))
            {
                exitCode = 2;
                return "Failed to write pin\n";
            }
        }
        return "OK\n";
    }

    if (command == "mode")
    {
        if (parseResult.remainingArguments.size() < 3)
        {
            exitCode = 1;
            return "Usage: gpio mode [pin[,pin...]] [input|output]\n";
        }

        GpioMode mode;
        if (parseResult.remainingArguments[2] == "input")
        {
            mode = GpioMode::Input;
        }
        else if (parseResult.remainingArguments[2] == "output")
        {
            mode = GpioMode::Output;
        }
        else
        {
            exitCode = 1;
            return "Invalid mode. Use input|output\n";
        }

        auto inputPins = parsePins_(parseResult.remainingArguments[1]);
        for (const auto &pinInput : inputPins)
        {
            ETString resolved;
            ETString error;
            if (!resolveAndAuthorize_(pinInput, GpioOperation::Mode, resolved, error))
            {
                exitCode = 1;
                return error + "\n";
            }

            if (!gpio_.setMode(resolved, mode))
            {
                exitCode = 2;
                return "Failed to set pin mode\n";
            }
        }
        return "OK\n";
    }

    if (command == "deny")
    {
        if (parseResult.remainingArguments.size() < 2)
        {
            exitCode = 1;
            return "Usage: gpio deny [pin[,pin...]] [read|write|mode|include|exclude ...] [--protected]\n";
        }

        GpioExclusionRule opMask;
        opMask.denyRead = false;
        opMask.denyWrite = false;
        opMask.denyMode = false;
        opMask.denyInclude = false;
        opMask.denyExclude = false;
        ETString parseError = parseOperationTokens_(parseResult.remainingArguments, 2, opMask);
        if (!parseError.empty())
        {
            exitCode = 1;
            return parseError + "\n";
        }
        if (!opMask.denyRead && !opMask.denyWrite && !opMask.denyMode && !opMask.denyInclude && !opMask.denyExclude)
        {
            opMask.denyRead = true;
            opMask.denyWrite = true;
            opMask.denyMode = true;
        }

        auto inputPins = parsePins_(parseResult.remainingArguments[1]);
        for (const auto &pinInput : inputPins)
        {
            ETString resolved;
            ETString error;
            if (!resolveAndValidatePin_(pinInput, resolved, error))
            {
                exitCode = 1;
                return error + "\n";
            }

            auto rules = policy_.exclusions();
            bool protectedRule = false;
            for (const auto &existing : rules)
            {
                if (existing.pinId == resolved && existing.passwordProtected)
                {
                    protectedRule = true;
                    break;
                }
            }

            GpioExclusionRule rule;
            rule.pinId = resolved;
            rule.denyRead = opMask.denyRead;
            rule.denyWrite = opMask.denyWrite;
            rule.denyMode = opMask.denyMode;
            rule.denyInclude = opMask.denyInclude;
            rule.denyExclude = opMask.denyExclude;
            rule.passwordProtected = protectedRequested || protectedRule;

            if (requiresAuthentication_(rule) && !isAuthenticated_(context))
            {
                exitCode = 1;
                return "Authentication required for protected policy change\n";
            }

            ETString reason;
            if (!policy_.addExclusion(rule, reason))
            {
                exitCode = 2;
                if (reason.empty())
                {
                    reason = "Failed to add deny rule";
                }
                return reason + "\n";
            }
        }
        return "Deny rule applied\n";
    }

    if (command == "allow")
    {
        if (parseResult.remainingArguments.size() < 2)
        {
            exitCode = 1;
            return "Usage: gpio allow [pin[,pin...]] [read|write|mode|include|exclude ...]\n";
        }

        GpioExclusionRule opMask;
        opMask.denyRead = false;
        opMask.denyWrite = false;
        opMask.denyMode = false;
        opMask.denyInclude = false;
        opMask.denyExclude = false;
        ETString parseError = parseOperationTokens_(parseResult.remainingArguments, 2, opMask);
        if (!parseError.empty())
        {
            exitCode = 1;
            return parseError + "\n";
        }

        bool hasExplicitOps = false;
        for (size_t i = 2; i < parseResult.remainingArguments.size(); i++)
        {
            if (parseResult.remainingArguments[i] != "--protected")
            {
                hasExplicitOps = true;
                break;
            }
        }
        if (!hasExplicitOps)
        {
            opMask.denyRead = true;
            opMask.denyWrite = true;
            opMask.denyMode = true;
        }

        auto inputPins = parsePins_(parseResult.remainingArguments[1]);
        for (const auto &pinInput : inputPins)
        {
            ETString resolved;
            ETString error;
            if (!resolveAndValidatePin_(pinInput, resolved, error))
            {
                exitCode = 1;
                return error + "\n";
            }

            auto rules = policy_.exclusions();
            const GpioExclusionRule *existingRule = nullptr;
            bool protectedRule = false;
            for (const auto &rule : rules)
            {
                if (rule.pinId == resolved)
                {
                    existingRule = &rule;
                    if (rule.passwordProtected)
                    {
                        protectedRule = true;
                    }
                }
            }

            if (protectedRule && !isAuthenticated_(context))
            {
                exitCode = 1;
                return "Authentication required for protected policy change\n";
            }

            GpioExclusionRule rule;
            rule.pinId = resolved;
            if (existingRule != nullptr)
            {
                rule = *existingRule;
            }

            if (opMask.denyRead)
            {
                rule.denyRead = false;
            }
            if (opMask.denyWrite)
            {
                rule.denyWrite = false;
            }
            if (opMask.denyMode)
            {
                rule.denyMode = false;
            }
            if (opMask.denyInclude)
            {
                rule.denyInclude = false;
            }
            if (opMask.denyExclude)
            {
                rule.denyExclude = false;
            }

            bool emptyRule = !rule.denyRead && !rule.denyWrite && !rule.denyMode && !rule.denyInclude && !rule.denyExclude;
            ETString reason;
            if (emptyRule)
            {
                if (!policy_.addExclusion(rule, reason))
                {
                    exitCode = 2;
                    if (reason.empty())
                    {
                        reason = "Failed to add allow rule";
                    }
                    return reason + "\n";
                }
            }
            else
            {
                if (!policy_.addExclusion(rule, reason))
                {
                    exitCode = 2;
                    if (reason.empty())
                    {
                        reason = "Failed to update allow rule";
                    }
                    return reason + "\n";
                }
            }
        }

        return "Allow rule applied\n";
    }

    exitCode = 1;
    return "Unknown subcommand\n" + usage("gpio");
}

bool gpio::isAuthenticated_(CommandContext *context) const
{
    if (auth_ == nullptr)
    {
        return true;
    }
    if (context == nullptr)
    {
        return false;
    }
    auto it = context->variables.find(SESSION_KEY_AUTHENTICATED);
    if (it == context->variables.end())
    {
        return false;
    }
    if (it->second != "1")
    {
        return false;
    }

    const uint64_t ttlMs = static_cast<uint64_t>(ET_GPIO_AUTH_TTL_MS);
    if (ttlMs == 0)
    {
        return true;
    }

    auto expiryIt = context->variables.find(SESSION_KEY_AUTH_UNTIL_MS);
    if (expiryIt == context->variables.end())
    {
        return false;
    }

    uint64_t expiresAt = static_cast<uint64_t>(ETString::toull(expiryIt->second.c_str()));
    if (nowMs_() > expiresAt)
    {
        context->variables[SESSION_KEY_AUTHENTICATED] = "0";
        context->variables.erase(SESSION_KEY_AUTH_UNTIL_MS);
        return false;
    }

    return true;
}

bool gpio::requiresAuthentication_(const GpioExclusionRule &rule) const
{
    return auth_ != nullptr && rule.passwordProtected;
}

ETString gpio::parseOperationTokens_(const ETVector<ETString> &tokens, size_t startIndex, GpioExclusionRule &rule)
{
    for (size_t i = startIndex; i < tokens.size(); i++)
    {
        if (tokens[i] == "read" || tokens[i] == "r")
        {
            rule.denyRead = true;
        }
        else if (tokens[i] == "write" || tokens[i] == "w")
        {
            rule.denyWrite = true;
        }
        else if (tokens[i] == "mode" || tokens[i] == "m")
        {
            rule.denyMode = true;
        }
        else if (tokens[i] == "include" || tokens[i] == "i")
        {
            rule.denyInclude = true;
        }
        else if (tokens[i] == "exclude" || tokens[i] == "e")
        {
            rule.denyExclude = true;
        }
        else
        {
            return "Unknown exclusion token: " + tokens[i];
        }
    }
    return "";
}

ETVector<ETString> gpio::parsePins_(const ETString &rawPins) const
{
    ETVector<ETString> pins;
    auto parts = split(rawPins, ",");
    for (const auto &part : parts)
    {
        ETString cleaned = part.trim();
        if (!cleaned.empty())
        {
            pins.push_back(cleaned);
        }
    }
    return pins;
}

bool gpio::resolveAndValidatePin_(const ETString &inputPin, ETString &resolvedPin, ETString &error) const
{
    if (!policy_.resolveIdentifier(inputPin, resolvedPin))
    {
        error = "Unknown pin identifier";
        return false;
    }

    if (!gpio_.exists(resolvedPin))
    {
        error = "Pin not available on this board";
        return false;
    }

    return true;
}

bool gpio::resolveAndAuthorize_(const ETString &inputPin, GpioOperation operation, ETString &resolvedPin, ETString &error)
{
    if (!policy_.resolveIdentifier(inputPin, resolvedPin))
    {
        error = "Unknown pin identifier";
        return false;
    }

    if (!gpio_.exists(resolvedPin))
    {
        error = "Pin not available on this board";
        return false;
    }

    if (!policy_.isOperationAllowed(resolvedPin, operation, error))
    {
        if (error.empty())
        {
            error = "Operation blocked by policy";
        }
        return false;
    }

    return true;
}

ETString gpio::formatRule_(const GpioExclusionRule &rule) const
{
    ETString line = rule.pinId + " [";
    bool hasDeniedOperation = false;
    if (rule.denyRead)
    {
        line += "read ";
        hasDeniedOperation = true;
    }
    if (rule.denyWrite)
    {
        line += "write ";
        hasDeniedOperation = true;
    }
    if (rule.denyMode)
    {
        line += "mode ";
        hasDeniedOperation = true;
    }
    if (rule.denyInclude)
    {
        line += "include ";
        hasDeniedOperation = true;
    }
    if (rule.denyExclude)
    {
        line += "exclude ";
        hasDeniedOperation = true;
    }
    if (!hasDeniedOperation)
    {
        line += "allow ";
    }
    line += "]";
    if (rule.forced)
    {
        line += " forced";
    }
    if (rule.passwordProtected)
    {
        line += " protected";
    }
    return line;
}

uint64_t gpio::nowMs_() const
{
#if defined(ARDUINO)
    return static_cast<uint64_t>(millis());
#elif defined(ESP_PLATFORM)
    return static_cast<uint64_t>(esp_timer_get_time() / 1000ULL);
#else
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
#endif
}