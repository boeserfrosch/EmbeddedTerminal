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
            ETMap<ETString, ETString> options;
            ETString remainingArguments;
            ETString errorMessage;
        };

        void addOption(const ETString &shortOpt, const ETString &longOpt, const ETString &description, bool requiresValue = false);
        void addRequiredRemainingArgument(const ETString &name);
        ParseResult parse(const ETString &input);

    private:
        struct OptionDefinition
        {
            ETString shortOpt;
            ETString longOpt;
            ETString description;
            bool requiresValue;
        };
        ETVector<OptionDefinition> _options;
        ETVector<ETString> _requiredRemainingArguments;
    };
} // namespace EmbeddedTerminal

#endif // OPTION_PARSER_H