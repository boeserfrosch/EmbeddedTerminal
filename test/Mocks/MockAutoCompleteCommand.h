#ifndef MOCK_AUTO_COMPLETE_COMMAND_H
#define MOCK_AUTO_COMPLETE_COMMAND_H

#include "../../src/interfaces/ICommand.h"
#include "../../src/interfaces/IAutoCompleter.h"
#include "../../src/ETTypes.h"

class MockAutoCompleteCommand : public EmbeddedTerminal::ICommand
{
public:
    ETString lastKeyword;
    ETString lastAdditional;

    // Store test suggestions
    ETVector<ETString> testSuggestions;

    ETString usage(const ETString &keyword) override
    {
        lastKeyword = keyword;
        return "Usage: " + keyword;
    }

    ETString trigger(const ETString &keyword, const ETString &additional) override
    {
        lastKeyword = keyword;
        lastAdditional = additional;
        return "Triggered: " + keyword + " " + additional;
    }

    ETVector<ETString> getSuggestions(const ETString &partial) override
    {
        ETVector<ETString> results;

        // Return test suggestions that match the partial
        for (const auto &suggestion : testSuggestions)
        {
            if (suggestion.startsWith(partial))
            {
                results.push_back(suggestion);
            }
        }

        return results;
    }
};

#endif // MOCK_AUTO_COMPLETE_COMMAND_H
