#pragma once

#if defined(ESP_PLATFORM)

#include "interfaces/ITerminalStream.h"

extern "C"
{
#include "driver/uart.h"
}

#include <cstdarg>
#include <cstdio>

namespace EmbeddedTerminal
{
    class ESPIDFTerminalStream : public ITerminalStream
    {
    public:
        ESPIDFTerminalStream(uart_port_t uartPort)
            : uartPort_(uartPort)
        {
        }

        bool available() override
        {
            size_t bufferedSize = 0;
            if (uart_get_buffered_data_len(uartPort_, &bufferedSize) != ESP_OK)
            {
                return false;
            }
            return bufferedSize > 0;
        }

        ETString readAll() override
        {
            ETString result;
            size_t bufferedSize = 0;
            if (uart_get_buffered_data_len(uartPort_, &bufferedSize) != ESP_OK || bufferedSize == 0)
            {
                return result;
            }

            ETVector<uint8_t> buffer(bufferedSize);
            int read = uart_read_bytes(uartPort_, buffer.data(), static_cast<uint32_t>(buffer.size()), 0);
            if (read <= 0)
            {
                return result;
            }

            result.reserve(static_cast<size_t>(read));
            for (int i = 0; i < read; i++)
            {
                result += static_cast<char>(buffer[static_cast<size_t>(i)]);
            }
            return result;
        }

        void print(const ETString &s) override
        {
            if (s.empty())
            {
                return;
            }
            uart_write_bytes(uartPort_, s.c_str(), static_cast<size_t>(s.length()));
        }

        void printf(const char *fmt, ...) override
        {
            char buffer[256];
            va_list args;
            va_start(args, fmt);
            vsnprintf(buffer, sizeof(buffer), fmt, args);
            va_end(args);
            print(ETString(buffer));
        }

        void printTo(TerminalChannel channel, const ETString &s) override
        {
            (void)channel;
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

    private:
        uart_port_t uartPort_;
    };
}

#endif
