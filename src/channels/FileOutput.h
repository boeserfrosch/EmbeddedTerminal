#pragma once
#include "interfaces/ICommandRuntime.h"
#include "ETFile.h"

namespace EmbeddedTerminal
{
    namespace Channels
    {
        class FileOutput : public IOutputChannel
        {
        public:
            FileOutput(const ETFile &file);
            ~FileOutput();

            void print(const ETString &s) override;

        private:
            ETFile file_;
        };
    } // namespace Channels
} // namespace EmbeddedTerminal