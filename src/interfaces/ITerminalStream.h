#ifndef I_TERMINAL_STREAM_H
#define I_TERMINAL_STREAM_H

#include "../ETTypes.h"
#include <cstdarg>
#include <cstdio>

namespace EmbeddedTerminal
{

    /**
     * @brief Enum representing the output channels of the terminal stream.
     */
    enum class TerminalChannel
    {
        StdOut,
        StdErr
    };

    /**
     * @brief Interface for terminal input/output streams.
     */
    class ITerminalStream
    {
    public:
        virtual ~ITerminalStream() = default;

        /**
         * @brief Checks if there is input available to read from the stream.
         * @return True if input is available, false otherwise.
         */
        virtual bool available() = 0;

        /**
         * @brief Reads all available input from the stream as a string.
         * @return The input read from the stream as an ETString.
         */
        virtual ETString readAll() = 0;

        /**
         * @brief Prints a string to the terminal stream
         */
        virtual void print(const ETString &s) = 0;

        /**
         * @brief Prints a formatted string to the terminal stream (like printf).
         * @param fmt The format string.
         * @param ... Additional arguments for the format string.
         */
        virtual void printf(const char *fmt, ...) = 0;

        /**
         * @brief Prints a string to the specified terminal channel (stdout or stderr).
         * @param channel The terminal channel to print to.
         * @param s The string to print.
         */
        virtual void printTo(TerminalChannel channel, const ETString &s)
        {
            (void)channel;
            print(s);
        }

        /**
         * @brief Prints a formatted string to the specified terminal channel (stdout or stderr).
         * @param channel The terminal channel to print to.
         * @param fmt The format string.
         * @param ... Additional arguments for the format string.
         */
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
