#ifndef ARDUINO_STREAM_H
#define ARDUINO_STREAM_H

#if defined(ARDUINO) || defined(ESP_PLATFORM)

#include <Arduino.h>
#include "../interfaces/ITerminalStream.h"
#include "../ETTypes.h"

namespace EmbeddedTerminal
{
    class ArduinoStream : public ITerminalStream
    {
    protected:
        Stream &stream_;

    public:
        ArduinoStream(Stream &stream) : stream_(stream) {}

        bool available() override
        {
            return stream_.available() > 0;
        }

        ETString readAll() override
        {
            ETString result;
            while (stream_.available())
            {
                char c = stream_.read();
                result += c;
            }
            return result;
        }

        void print(const ETString &s) override
        {
            stream_.print(s.c_str());
        }

        void printf(const char *fmt, ...) override
        {
            char buffer[256];
            va_list args;
            va_start(args, fmt);
            vsnprintf(buffer, sizeof(buffer), fmt, args);
            va_end(args);
            stream_.print(buffer);
        }

        void printTo(TerminalChannel channel, const ETString &s) override
        {
            (void)channel; // Ignoring channel for Arduino Stream
            print(s);
        }

        void printfTo(TerminalChannel channel, const char *fmt, ...) override
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

#endif // ARDUINO || ESP_PLATFORM

#endif // ARDUINO_STREAM_H
