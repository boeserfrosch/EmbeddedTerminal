#include "BufferedInOut.h"

void EmbeddedTerminal::Channels::BufferedInOut::print(const ETString &text)
{
    buffer_ += text;
}

const ETString &EmbeddedTerminal::Channels::BufferedInOut::getBuffer() const
{
    return buffer_;
}

bool EmbeddedTerminal::Channels::BufferedInOut::available()
{
    return readPosition_ < buffer_.length();
}

ETString EmbeddedTerminal::Channels::BufferedInOut::readAll()
{
    if (readPosition_ >= buffer_.length())
    {
        return "";
    }
    ETString result = buffer_.substr(readPosition_);
    readPosition_ += result.length();
    return result;
}