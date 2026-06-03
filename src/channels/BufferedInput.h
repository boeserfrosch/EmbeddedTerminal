#pragma once
#include "interfaces/ICommandRuntime.h"

namespace EmbeddedTerminal
{
    namespace Channels
    {
        class BufferedInput : public IInputChannel
        {
        public:
            BufferedInput(const ETString &content);
            virtual ~BufferedInput();

            // IInputChannel implementation
            bool available() override;
            ETString readAll() override;

        private:
            ETString content_;
            size_t position_;
        };
    }
} // namespace EmbeddedTerminal
