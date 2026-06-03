#ifndef MOCKCOMMAND_H
#define MOCKCOMMAND_H

#include "../../src/interfaces/ICommand.h"
#include "../../src/ETTypes.h"

using namespace EmbeddedTerminal;
class MockCommand : public ICommand
{
public:
    ETString lastKeyword;
    ETString lastAdditional;
    ETString usage(const ETString &keyword) const override
    {
        return "Usage: " + keyword;
    }

    CommandResult invoke(CommandInvocation &invocation) override
    {
        lastKeyword = invocation.keyword;
        lastAdditional = join(invocation.arguments, " ");
        invocation.streams.output.print(lastKeyword + " " + lastAdditional);
        return CommandResult::completed(0);
    }

    CommandResult resume(CommandInvocation &invocation) override
    {
        return CommandResult::completed(0);
    }
};

class StreamingMockCommandTokenCollector : public ICommand
{
public:
    StreamingMockCommandTokenCollector() = default;
    StreamingMockCommandTokenCollector(bool resetOnExecute) : resetOnExecute_(resetOnExecute)
    {
    }

    ETVector<ETString> collectedTokens;

    ETString usage(const ETString &keyword) const override
    {
        (void)keyword;
        return "Usage: " + keyword;
    }

    CommandResult invoke(CommandInvocation &invocation) override
    {
        if (resetOnExecute_)
        {
            collectedTokens.clear();
        }

        collectedTokens.insert(collectedTokens.end(), invocation.arguments.begin(), invocation.arguments.end());
        invocation.streams.output.print("Collected " + toETString(collectedTokens.size()) + " tokens");
        return CommandResult::completed(0);
    }

    CommandResult resume(CommandInvocation &invocation) override
    {
        return invoke(invocation);
    }

private:
    bool resetOnExecute_ = false;
};

class CapturingCommand : public ICommand
{
public:
    ETString response;
    int exitCode = 0;
    ETString lastKeyword;

    CapturingCommand(const ETString &responseText = "ok", int code = 0) : response(responseText), exitCode(code) {}

    ETString usage(const ETString &keyword) const override
    {
        (void)keyword;
        return "";
    }

    CommandResult invoke(CommandInvocation &invocation) override
    {
        lastKeyword = invocation.keyword;
        invocation.streams.output.print(response);
        return CommandResult::completed(exitCode);
    }
};
#endif // MOCKCOMMAND_H
