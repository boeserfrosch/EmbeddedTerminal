#include "interfaces/ICommandRuntime.h"
#include "../Mocks/MockTerminal.h"

namespace EmbeddedTerminal
{

    class EmptyInputChannel : public EmbeddedTerminal::IInputChannel
    {
    public:
        bool available() override { return inputPos < data.length(); }
        ETString readAll() override
        {
            ETString value = data.substr(inputPos);
            inputPos = data.length();
            return value;
        }

        void print(const ETString &s)
        {
            data += s;
        }

    protected:
        size_t inputPos = 0;
        ETString data = "";
    };

    class BufferedInputChannel : public EmbeddedTerminal::IInputChannel
    {
    public:
        ETString buffer;

        bool available() override { return !buffer.empty(); }

        ETString readAll() override
        {
            ETString value = buffer;
            buffer = "";
            return value;
        }
    };

    class StreamBackedOutputChannel : public EmbeddedTerminal::IOutputChannel
    {
    public:
        StreamBackedOutputChannel(MockStream &streamRef, EmbeddedTerminal::TerminalChannel channelRef) : stream(streamRef), channel(channelRef) {}

        void print(const ETString &s) override
        {
            stream.printTo(channel, s);
        }

    private:
        MockStream &stream;
        EmbeddedTerminal::TerminalChannel channel;
    };

    class DebugOutputChannel : public EmbeddedTerminal::IOutputChannel
    {
    public:
        void print(const ETString &s) override
        {
            printf("DebugOutputChannel received: %s\n", s.c_str());
            debugOutput += s;
            printf("DebugOutputChannel current buffer: %s\n", debugOutput.c_str());
        }

        bool contains(const ETString &s) const
        {
            return debugOutput.find(s) != ETString::npos;
        }

        bool endsWith(const ETString &s) const
        {
            return debugOutput.endsWith(s);
        }

        bool empty() const
        {
            return debugOutput.empty();
        }

        size_t length() const
        {
            return debugOutput.length();
        }

        const char *c_str() const
        {
            return debugOutput.c_str();
        }

        operator const ETString &() const
        {
            return debugOutput;
        }

        operator const char *() const
        {
            return debugOutput.c_str();
        }

        ETString debugOutput;
    };

    struct TestCommandInvocationHandle
    {
        MockTerminal terminal;
        MockStream stream;
        ETMap<ETString, ETString> variables;
        CommandContext context;
        EmptyInputChannel input;
        DebugOutputChannel output;
        DebugOutputChannel error;
        CommandInvocation invocation;

        TestCommandInvocationHandle(const ETString &keyword, const ETVector<ETString> &args = {}) : terminal(), stream(), variables(), context(variables, 0, true), input(), output(), error(), invocation(keyword, args, context, input, output, error)
        {
            terminal.setInputStream(&input);
            terminal.setOutputStream(&output);
            terminal.setErrorStream(&error);
        }
    };

} // namespace EmbeddedTerminal
