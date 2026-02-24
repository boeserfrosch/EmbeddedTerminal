#include "OptionParser.h"

void EmbeddedTerminal::OptionParser::addOption(const ETString &shortOpt, const ETString &longOpt, const ETString &description, bool requiresValue)
{
    _options.push_back({shortOpt, longOpt, description, requiresValue});
}

EmbeddedTerminal::OptionParser::ParseResult EmbeddedTerminal::OptionParser::parse(const ETString &input)
{
    ParseResult result;
    ETString remaining = input;

    for (const auto &opt : _options)
    {
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
            // Found an option
            if (opt.requiresValue)
            {
                // Extract value
                size_t valueStart = pos + optNameLength;
                if (valueStart >= remaining.length())
                {
                    result.success = false;
                    result.errorMessage = "Option " + optName + " requires a value";
                    return result;
                }

                // Skip any whitespace before the value
                while (valueStart < remaining.length() && remaining[valueStart] == ' ')
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
                result.options[opt.longOpt] = value;
                remaining.erase(pos, valueEnd - pos + (isQuoted ? 1 : 0)); // Remove option and value from remaining
            }
            else
            {
                result.options[opt.longOpt] = "true";
                remaining.erase(pos, optNameLength);
            }
        }
    }

    // Check for required remaining arguments
    for (const auto &reqArg : _requiredRemainingArguments)
    {
        if (remaining.trim().empty())
        {
            result.success = false;
            result.errorMessage = "Missing required argument: " + reqArg;
            return result;
        }
        auto value = remaining.trim().substr(0, remaining.trim().find(' '));
        if (value.empty())
        {
            result.success = false;
            result.errorMessage = "Missing required argument: " + reqArg;
            return result;
        }
        result.options[reqArg] = value;
        remaining.erase(0, value.length() + 1); // +1 to account for the space
    }

    result.success = true;
    result.remainingArguments = remaining.trim();
    return result;
}

void EmbeddedTerminal::OptionParser::addRequiredRemainingArgument(const ETString &name)
{
    _requiredRemainingArguments.push_back(name);
}