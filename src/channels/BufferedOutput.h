#pragma once
#include "interfaces/ICommandRuntime.h"

namespace EmbeddedTerminal
{
    namespace Channels
    {
        class BufferedOutput : public IOutputChannel
        {
        public:
            BufferedOutput();
            virtual ~BufferedOutput();

            void print(const ETString &s) override;
            const ETString &getBuffer() const override;

        private:
            ETString buffer_;
        };
    } // namespace Channels
} // namespace EmbeddedTerminal