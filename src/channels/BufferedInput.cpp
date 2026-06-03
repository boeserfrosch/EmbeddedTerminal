#include "BufferedInput.h"

namespace EmbeddedTerminal
{
    namespace Channels
    {
        BufferedInput::BufferedInput(const ETString &content) : content_(content), position_(0) {}

        BufferedInput::~BufferedInput() {}

        bool BufferedInput::available()
        {
            return position_ < content_.length();
        }

        ETString BufferedInput::readAll()
        {
            if (position_ >= content_.length())
            {
                return "";
            }
            ETString result = content_.substr(position_);
            position_ = content_.length();
            return result;
        }
    } // namespace Channels
} // namespace EmbeddedTerminal