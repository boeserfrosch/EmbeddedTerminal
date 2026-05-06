#include "OptionParser.h"

void EmbeddedTerminal::OptionParser::addOption(const ETString &shortOpt, const ETString &longOpt, const ETString &description, bool requiresValue, bool isRequired)
{
    options_.push_back({shortOpt, longOpt, description, requiresValue, isRequired});
}

EmbeddedTerminal::OptionParser::ParseResult EmbeddedTerminal::OptionParser::parse(const ETString &input)
{
    ParseResult result;
    ETString remaining = input;

    for (const auto &opt : options_)
    {
        // Keep searching for this option until we don't find it anymore
        // This allows multiple instances of the same flag: -i eth0 -i eth1
        bool foundOption = true;
        while (foundOption)
        {
            foundOption = false;
            size_t pos = remaining.find(opt.longOpt);
            size_t optNameLength = opt.longOpt.length();
            ETString optName = opt.longOpt;
            if (pos == ETString::npos)
            {
                pos = remaining.find(opt.shortOpt);
                optNameLength = opt.shortOpt.length();
                optName = opt.shortOpt;
            }

            if (pos != ETString::npos)
            {
                foundOption = true;
                // Found an option
                if (opt.requiresValue)
                {
                    // Extract value
                    size_t valueStart = pos + optNameLength;
                    if (valueStart >= remaining.trim().length())
                    {
                        result.success = false;
                        result.errorMessage = "Option " + optName + " requires a value";
                        return result;
                    }

                    // Skip any whitespace before the value
                    while (valueStart < remaining.trim().length() && remaining[valueStart] == ' ')
                    {
                        valueStart++;
                    }
                    // Read until next space or end of string or quote (for quoted values)
                    size_t valueEnd = remaining.length();
                    bool isQuoted = valueStart < remaining.length() && remaining[valueStart] == '"';
                    if (remaining[valueStart] == '"')
                    {
                        valueStart++; // Skip opening quote
                        valueEnd = remaining.find('"', valueStart);
                        if (valueEnd == ETString::npos)
                        {
                            result.success = false;
                            result.errorMessage = "Unmatched quote in option value";
                            return result;
                        }
                    }
                    else
                    {
                        valueEnd = remaining.find(' ', valueStart);
                    }
                    ETString value = remaining.substr(valueStart, valueEnd - valueStart).trim();
                    result.options[opt.longOpt].push_back(value);
                    if (!opt.shortOpt.empty() && opt.shortOpt != opt.longOpt)
                    {
                        result.options[opt.shortOpt].push_back(value);
                    }
                    remaining.erase(pos, valueEnd - pos + (isQuoted ? 1 : 0)); // Remove option and value from remaining
                }
                else
                {
                    result.options[opt.longOpt].push_back("true");
                    if (!opt.shortOpt.empty() && opt.shortOpt != opt.longOpt)
                    {
                        result.options[opt.shortOpt].push_back("true");
                    }
                    remaining.erase(pos, optNameLength);
                }
            }
        }
    }

    for (const auto &opt : options_)
    {
        if (!opt.isRequired)
        {
            continue;
        }

        const bool hasLongOption = result.options.find(opt.longOpt) != result.options.end() && !result.options[opt.longOpt].empty();
        const bool hasShortOption = !opt.shortOpt.empty() && result.options.find(opt.shortOpt) != result.options.end() && !result.options[opt.shortOpt].empty();
        if (!hasLongOption && !hasShortOption)
        {
            result.success = false;
            result.errorMessage = "Missing required option: " + (opt.longOpt.empty() ? opt.shortOpt : opt.longOpt);
            return result;
        }
    }

    // Check for required remaining arguments
    ETVector<ETString> positionalArguments = split(remaining.trim(), " ");
    ETVector<ETString> filteredArguments;
    for (const auto &argument : positionalArguments)
    {
        if (!argument.trim().empty())
        {
            filteredArguments.push_back(argument.trim());
        }
    }

    size_t requiredArgumentsRemaining = 0;
    for (const auto &argumentDefinition : remainingArguments_)
    {
        if (argumentDefinition.second)
        {
            requiredArgumentsRemaining++;
        }
    }

    for (size_t index = 0; index < remainingArguments_.size(); ++index)
    {
        const auto &argumentDefinition = remainingArguments_[index];
        const bool isRequired = argumentDefinition.second;
        const size_t requiredAfterCurrent = (requiredArgumentsRemaining > 0 && isRequired) ? (requiredArgumentsRemaining - 1) : requiredArgumentsRemaining;

        if (filteredArguments.empty())
        {
            if (isRequired)
            {
                result.success = false;
                result.errorMessage = "Missing required argument: " + argumentDefinition.first;
                return result;
            }

            continue;
        }

        if (!isRequired)
        {
            if (filteredArguments.size() <= requiredAfterCurrent)
            {
                continue;
            }

            result.options[argumentDefinition.first].push_back(filteredArguments.front());
            filteredArguments.erase(filteredArguments.begin());
            continue;
        }

        if (filteredArguments.size() <= requiredAfterCurrent)
        {
            result.success = false;
            result.errorMessage = "Missing required argument: " + argumentDefinition.first;
            return result;
        }

        result.options[argumentDefinition.first].push_back(filteredArguments.front());
        filteredArguments.erase(filteredArguments.begin());
        requiredArgumentsRemaining--;
    }

    result.success = true;
    result.remainingArguments = join(filteredArguments, " ");
    return result;
}

void EmbeddedTerminal::OptionParser::addOptionalRemainingArgument(const ETString &name)
{
    remainingArguments_.push_back({name, false});
}

void EmbeddedTerminal::OptionParser::addRequiredRemainingArgument(const ETString &name)
{
    remainingArguments_.push_back({name, true});
}