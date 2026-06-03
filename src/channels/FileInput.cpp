#include "FileInput.h"

namespace EmbeddedTerminal::Channels
{
    FileInput::FileInput(const ETFile &file) : file_(file) {}

    FileInput::~FileInput() {}

    bool FileInput::available()
    {
        return file_.position() < file_.size();
    }

    ETString FileInput::readAll()
    {
        return file_.readAll();
    }
} // namespace EmbeddedTerminal::Channels