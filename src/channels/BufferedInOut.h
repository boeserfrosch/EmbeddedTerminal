#pragma once
#include "interfaces/ICommandRuntime.h"

namespace EmbeddedTerminal
{
    namespace Channels
    {
        class BufferedInOut : public IInputChannel, public IOutputChannel
        {
        public:
            void print(const ETString &text) override;
            const ETString &getBuffer() const override;
            bool available() override;
            ETString readAll() override;

        private:
            ETString buffer_;
            size_t readPosition_ = 0;
        };
    }
}