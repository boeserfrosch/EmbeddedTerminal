#include "TeeOutput.h"

namespace EmbeddedTerminal::Channels
{
    TeeOutput::TeeOutput(ITerminalStream &stream, TerminalChannel channel, ETString &capture)
        : stream_(stream), channel_(channel), capture_(capture)
    {
    }

    void TeeOutput::print(const ETString &s)
    {
        capture_ += s;
        stream_.printTo(channel_, s);
    }
} // namespace EmbeddedTerminal::Channels