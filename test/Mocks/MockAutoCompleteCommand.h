#ifndef MOCK_AUTO_COMPLETE_COMMAND_H
#define MOCK_AUTO_COMPLETE_COMMAND_H

#include "interfaces/ICommand.h"
#include "interfaces/IAutoCompleter.h"
#include "ETTypes.h"

class MockAutoCompleteCommand : public EmbeddedTerminal::ICommand
{
public:
    ETString lastKeyword;
    ETString lastAdditional;

    // Store test suggestions
    ETVector<ETString> testSuggestions;

    ETString usage(const ETString &keyword) const override
    {
        return "Usage: " + keyword;
    }

    CommandResult invoke(EmbeddedTerminal::CommandInvocation &invocation) override
    {
        lastKeyword = invocation.keyword;
        if (!invocation.arguments.empty())
        {
            lastAdditional = invocation.arguments[0];
        }
        invocation.streams.output.print("Invoked with keyword: " + lastKeyword + ", additional: " + lastAdditional + "\n");
        return CommandResult::completed(0);
    }

    ETVector<ETString> getSuggestions(const ETString &partial) const override
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
