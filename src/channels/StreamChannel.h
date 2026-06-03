
#ifndef STREAM_CHANNEL_H
#define STREAM_CHANNEL_H

#include "ETTypes.h"
#include "interfaces/ICommandRuntime.h"
#include "interfaces/ITerminalStream.h"

namespace EmbeddedTerminal
{
    namespace Channels
    {
        class StreamInput : public IInputChannel
        {
        public:
            StreamInput(ITerminalStream &stream) : stream_(stream) {
                                                   };
            bool available() override
            {
                return stream_.available();
            }
            ETString readAll() override
            {
                return stream_.readAll();
            }

        private:
            ITerminalStream &stream_;
        };

        class StreamOutput : public IOutputChannel
        {
        public:
            StreamOutput(ITerminalStream &stream, TerminalChannel channel) : stream_(stream), channel_(channel)
            {
            }
            void print(const ETString &s) override
            {
                stream_.printTo(channel_, s);
            }

        private:
            ITerminalStream &stream_;
            TerminalChannel channel_;
        };

    } // namespace Channels
} // namespace EmbeddedTerminal
#endif // STREAM_CHANNEL_H