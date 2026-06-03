#include "OptionParser.h"
#include "lang/LangAPI.h"

void EmbeddedTerminal::OptionParser::addOption(const ETString &shortOpt, const ETString &longOpt, const ETString &description, bool requiresValue, bool isRequired)
{
    options_.push_back({shortOpt, longOpt, description, requiresValue, isRequired});
}

EmbeddedTerminal::OptionParser::ParseResult EmbeddedTerminal::OptionParser::parse(const ETString &input)
{
    using namespace EmbeddedTerminal::Lang;
    auto tokens = Lexer().tokenize(input);
    for (const auto &token : tokens)
    {
        // Fail if one token is unterminated - this indicates a lexer error that should prevent parsing
        if ((token.type == TokenType::DOUBLE_QUOTE_STRING_LITERAL || token.type == TokenType::SINGLE_QUOTE_STRING_LITERAL) && !token.terminated)
        {
            ParseResult result;
            result.success = false;
            result.error = ParseErrorCode::UnmatchedQuote;
            return result;
        }
    }
    return parse(toTextVector(Lexer().tokenize(input)));
}

bool EmbeddedTerminal::OptionParser::matchRemainingArguments(const ETVector<ETString> &filteredArguments, ParseResult &result)
{
    size_t requiredCount = requiredRemainingArgumentsCount_;
    size_t consumedArguments = 0;

    // First check if we have enough arguments to satisfy the required ones: Die fast!
    if (filteredArguments.size() < requiredCount)
    {
        result.success = false;
        result.error = ParseErrorCode::MissingRequiredOption;
        return false;
    }

    size_t foundArguments = 0;
    for (size_t index = 0; index < remainingArguments_.size(); ++index)
    {
        const auto &argumentDefinition = remainingArguments_[index];
        const bool isRequired = argumentDefinition.second;
        const size_t argumentsLeft = filteredArguments.size() - consumedArguments;
        if (argumentsLeft == 0 && isRequired)
        {
            result.success = false;
            result.error = ParseErrorCode::MissingRequiredOption;
            return false;
        }

        if (!isRequired)
        {
            if (argumentsLeft > requiredCount)
            {
                result.options[argumentDefinition.first].push_back(filteredArguments[consumedArguments]);
                consumedArguments++;
                foundArguments++;
            }
            continue;
        }

        // For required arguments, ensure we have enough arguments remaining
        if (argumentsLeft < requiredCount)
        {
            result.success = false;
            result.error = ParseErrorCode::MissingRequiredArgument;
            return false;
        }

        result.options[argumentDefinition.first].push_back(filteredArguments[consumedArguments]);
        consumedArguments++;
        foundArguments++;
        requiredCount--;
    }
    result.remainingArguments = ETVector<ETString>(filteredArguments.begin() + consumedArguments, filteredArguments.end());
    return true;
}

EmbeddedTerminal::OptionParser::ParseResult EmbeddedTerminal::OptionParser::parse(const ETVector<ETString> &args)
{
    // Instead of joining the arguments into a single string and parsing, we can directly parse the vector of arguments
    ParseResult result;
    if (schemaInvalid_)
    {
        result.success = false;
        result.error = ParseErrorCode::InvalidOptionFormat;
        return result;
    }
    ETVector<ETString> remaining(args);
    ETMap<size_t, bool> consumedIndices; // To track which indices have been consumed
    // First, extract options from the input
    for (const auto &opt : options_)
    {
        bool foundOption = true;
        size_t searchStartIndex = 0;
        while (foundOption)
        {
            size_t pos = findFirstIndexOf(remaining, opt.longOpt, searchStartIndex);
            // size_t optNameLength = opt.longOpt.length();
            ETString optName = opt.longOpt;
            if (pos == ETString::npos)
            {
                pos = findFirstIndexOf(remaining, opt.shortOpt, searchStartIndex);
                // optNameLength = opt.shortOpt.length();
                optName = opt.shortOpt;
            }

            if (pos != ETString::npos)
            {
                searchStartIndex = pos + 1; // Continue searching after this option
                foundOption = true;
                if (opt.requiresValue)
                {
                    if (pos + 1 >= remaining.size())
                    {
                        result.success = false;
                        result.error = ParseErrorCode::MissingOptionValue;
                        return result;
                    }

                    ETString value = remaining[pos + 1];
                    result.options[opt.longOpt].push_back(value);
                    if (!opt.shortOpt.empty() && opt.shortOpt != opt.longOpt)
                    {
                        result.options[opt.shortOpt].push_back(value);
                    }
                    remaining[pos] = ETString();
                    remaining[pos + 1] = ETString();
                    consumedIndices[pos] = true;
                    consumedIndices[pos + 1] = true;
                }
                else
                {
                    result.options[opt.longOpt].push_back("true");
                    if (!opt.shortOpt.empty() && opt.shortOpt != opt.longOpt)
                    {
                        result.options[opt.shortOpt].push_back("true");
                    }
                    remaining[pos] = ETString();
                    consumedIndices[pos] = true;
                }
            }
            else
            {
                foundOption = false;
            }
        }
    }

    // Now check for required options
    if (!checkRequiredOptions(result.options))
    {
        result.success = false;
        result.error = ParseErrorCode::MissingRequiredOption;
        return result;
    }

    // Check for required remaining arguments
    ETVector<ETString> positionalArguments;
    for (size_t i = 0; i < remaining.size(); ++i)
    {
        if (consumedIndices.find(i) == consumedIndices.end())
        {
            positionalArguments.push_back(remaining[i]);
        }
    }

    bool hasAllRequiredArguments = matchRemainingArguments(positionalArguments, result);
    if (!hasAllRequiredArguments)
    {
        result.success = false;
        result.error = ParseErrorCode::MissingRequiredArgument;
        return result;
    }
    result.success = hasAllRequiredArguments;
    return result;
}

bool EmbeddedTerminal::OptionParser::checkRequiredOptions(const ETMap<ETString, ETVector<ETString>> &parsedOptions)
{
    for (const auto &opt : options_)
    {
        if (!opt.isRequired)
        {
            continue;
        }

        const bool hasLongOption = parsedOptions.find(opt.longOpt) != parsedOptions.end() && !parsedOptions.at(opt.longOpt).empty();
        const bool hasShortOption = !opt.shortOpt.empty() && parsedOptions.find(opt.shortOpt) != parsedOptions.end() && !parsedOptions.at(opt.shortOpt).empty();
        if (!hasLongOption && !hasShortOption)
        {
            return false;
        }
    }
    return true;
}

void EmbeddedTerminal::OptionParser::addOptionalRemainingArgument(const ETString &name)
{
    remainingArguments_.push_back({name, false});
}

void EmbeddedTerminal::OptionParser::addRequiredRemainingArgument(const ETString &name)
{
    if (requiredRemainingArgumentsCount_ < remainingArguments_.size())
    {
        schemaInvalid_ = true;
        schemaError_ = "Required positional arguments cannot be added after an optional positional argument";
        return;
    }
    remainingArguments_.push_back({name, true});
    requiredRemainingArgumentsCount_++;
}