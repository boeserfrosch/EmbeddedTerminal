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
        Stream &_stream;

    public:
        ArduinoStream(Stream &stream) : _stream(stream) {}

        bool available() override
        {
            return _stream.available() > 0;
        }

        ETString readAll() override
        {
            ETString result;
            while (_stream.available())
            {
                char c = _stream.read();
                result += c;
            }
            return result;
        }

        void print(const ETString &s) override
        {
            _stream.print(s.c_str());
        }

        void printf(const char *fmt, ...) override
        {
            char buffer[256];
            va_list args;
            va_start(args, fmt);
            vsnprintf(buffer, sizeof(buffer), fmt, args);
            va_end(args);
            _stream.print(buffer);
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
