#ifndef COMMAND_RUNTIME_TEST_UTILS_H
#define COMMAND_RUNTIME_TEST_UTILS_H

#include "../src/interfaces/ICommandRuntime.h"
#include "MockStream.h"

class EmptyInputChannel : public EmbeddedTerminal::IInputChannel
{
public:
    bool available() override { return false; }
    ETString readAll() override { return ""; }
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

#endif // COMMAND_RUNTIME_TEST_UTILS_H
