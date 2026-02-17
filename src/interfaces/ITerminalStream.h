#ifndef I_TERMINAL_STREAM_H
#define I_TERMINAL_STREAM_H
#include <string>

namespace EmbeddedTerminal
{

    class ITerminalStream
    {
    public:
        virtual ~ITerminalStream() = default;
        virtual bool available() = 0;
        virtual ETString readAll() = 0;
        virtual void print(const ETString &s) = 0;
        virtual void printf(const char *fmt, ...) = 0;
    };
} // namespace EmbeddedTerminal

#endif // I_TERMINAL_STREAM_H
