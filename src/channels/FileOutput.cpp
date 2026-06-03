#include "FileOutput.h"

namespace EmbeddedTerminal::Channels
{
    FileOutput::FileOutput(const ETFile &file) : file_(file) {}

    FileOutput::~FileOutput() {}

    void FileOutput::print(const ETString &s)
    {
        file_.writeAll(s);
    }
} // namespace EmbeddedTerminal::Channels