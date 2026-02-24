#ifndef MOCK_STREAM_H
#define MOCK_STREAM_H

#include <cstdint>
#include <cstring>
#include <cstdarg>
#include "../src/ETTypes.h"
#include "../src/interfaces/ITerminalStream.h"

using namespace EmbeddedTerminal;
class MockStream : public ITerminalStream
{
public:
    ETString inputBuffer;
    ETString outputBuffer;
    ETString stdoutBuffer;
    ETString stderrBuffer;
    size_t inputPos = 0;

    MockStream() : inputBuffer(), outputBuffer(), stdoutBuffer(), stderrBuffer(), inputPos(0) {}

    bool available() override
    {
        return inputPos < inputBuffer.length();
    }

    ETString readAll() override // Best practice: readAll or readRemaining
    {
        if (!available())
            return "";
        ETString result = inputBuffer.substr(inputPos);
        inputPos = inputBuffer.length();
        return result;
    }

    void print(const ETString &str) override
    {
        stdoutBuffer += str;
        outputBuffer += str;
    }

    void printf(const char *fmt, ...) override
    {
        char buf[256];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        ETString text(buf);
        stdoutBuffer += text;
        outputBuffer += text;
    }

    void printTo(TerminalChannel channel, const ETString &str) override
    {
        if (channel == TerminalChannel::StdErr)
        {
            stderrBuffer += str;
        }
        else
        {
            stdoutBuffer += str;
        }

        outputBuffer += str;
    }

    void printfTo(TerminalChannel channel, const char *fmt, ...) override
    {
        char buf[256];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);

        printTo(channel, ETString(buf));
    }
};

#endif // MOCK_STREAM_H
