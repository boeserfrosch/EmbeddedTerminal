#pragma once
#include "interfaces/ICommandRuntime.h"
#include "ETFile.h"

namespace EmbeddedTerminal
{
    namespace Channels
    {
        class FileInput : public IInputChannel
        {
        public:
            FileInput(const ETFile &file);
            ~FileInput();

            bool available() override;
            ETString readAll() override;

        private:
            ETFile file_;
        };
    } // namespace Channels

} // namespace EmbeddedTerminal
