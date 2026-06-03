#ifndef OPTION_PARSER_H
#define OPTION_PARSER_H

#include "ETTypes.h"

namespace EmbeddedTerminal
{
    class OptionParser
    {
    public:
        enum class ParseErrorCode
        {
            None = 0,
            UnmatchedQuote,
            InvalidOptionFormat,
            MissingOptionValue,
            UnknownOption,
            MissingRequiredOption,
            MissingRequiredArgument,
        };
        struct ParseResult
        {
            bool success = false;
            ParseErrorCode error = ParseErrorCode::None;
            ETMap<ETString, ETVector<ETString>> options;
            ETVector<ETString> remainingArguments = {};
        };

        void addOption(const ETString &shortOpt, const ETString &longOpt, const ETString &description, bool requiresValue = false, bool isRequired = false);
        void addRequiredRemainingArgument(const ETString &name);
        void addOptionalRemainingArgument(const ETString &name);
        ParseResult parse(const ETString &input);
        ParseResult parse(const ETVector<ETString> &args);

    private:
        bool checkRequiredOptions(const ETMap<ETString, ETVector<ETString>> &parsedOptions);
        bool matchRemainingArguments(const ETVector<ETString> &filteredArguments, ParseResult &result);

        struct OptionDefinition
        {
            ETString shortOpt;
            ETString longOpt;
            ETString description;
            bool requiresValue;
            bool isRequired;
        };
        ETVector<OptionDefinition> options_;
        ETVector<std::pair<ETString, bool>> remainingArguments_; // pair<name, isRequired>
        size_t requiredRemainingArgumentsCount_ = 0;
        bool schemaInvalid_ = false;
        ETString schemaError_;
    };
} // namespace EmbeddedTerminal

#endif // OPTION_PARSER_H