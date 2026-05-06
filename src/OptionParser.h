#ifndef OPTION_PARSER_H
#define OPTION_PARSER_H

#include "ETTypes.h"

namespace EmbeddedTerminal
{
    class OptionParser
    {
    public:
        struct ParseResult
        {
            bool success = false;
            ETMap<ETString, ETVector<ETString>> options;
            ETString remainingArguments;
            ETString errorMessage;
        };

        void addOption(const ETString &shortOpt, const ETString &longOpt, const ETString &description, bool requiresValue = false, bool isRequired = false);
        void addRequiredRemainingArgument(const ETString &name);
        void addOptionalRemainingArgument(const ETString &name);
        ParseResult parse(const ETString &input);

    private:
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
    };
} // namespace EmbeddedTerminal

#endif // OPTION_PARSER_H