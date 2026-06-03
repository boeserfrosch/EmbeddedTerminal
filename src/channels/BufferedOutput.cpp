#include "BufferedOutput.h"

namespace EmbeddedTerminal
{
    namespace Channels
    {
        BufferedOutput::BufferedOutput() : buffer_("") {}

        BufferedOutput::~BufferedOutput() {}

        void BufferedOutput::print(const ETString &s)
        {
            buffer_ += s;
        }

        const ETString &BufferedOutput::getBuffer() const
        {
            return buffer_;
        }
    } // namespace Channels
} // namespace EmbeddedTerminal