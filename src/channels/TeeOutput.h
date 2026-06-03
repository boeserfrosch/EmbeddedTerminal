#pragma once
#include "interfaces/ICommandRuntime.h"
#include "interfaces/ITerminalStream.h"

namespace EmbeddedTerminal
{
    namespace Channels
    {

        class TeeOutput : public IOutputChannel
        {
        public:
            TeeOutput(ITerminalStream &stream, TerminalChannel channel, ETString &capture);
            void print(const ETString &s) override;

        private:
            ITerminalStream &stream_;
            TerminalChannel channel_;
            ETString &capture_;
        };
    } // namespace Channels
} // namespace EmbeddedTerminal