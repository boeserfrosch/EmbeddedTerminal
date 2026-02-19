#ifndef I_TERMINAL_STREAM_H
#define I_TERMINAL_STREAM_H

#include "../ETTypes.h"
#include <cstdarg>
#include <cstdio>

namespace EmbeddedTerminal
{

    enum class TerminalChannel
    {
        StdOut,
        StdErr
    };

    class ITerminalStream
    {
    public:
        virtual ~ITerminalStream() = default;
        virtual bool available() = 0;
        virtual ETString readAll() = 0;
        virtual void print(const ETString &s) = 0;
        virtual void printf(const char *fmt, ...) = 0;

        virtual void printTo(TerminalChannel channel, const ETString &s)
        {
            (void)channel;
            print(s);
        }

        virtual void printfTo(TerminalChannel channel, const char *fmt, ...)
        {
            char buffer[256];
            va_list args;
            va_start(args, fmt);
            vsnprintf(buffer, sizeof(buffer), fmt, args);
            va_end(args);
            printTo(channel, ETString(buffer));
        }
    };
} // namespace EmbeddedTerminal

#endif // I_TERMINAL_STREAM_H
