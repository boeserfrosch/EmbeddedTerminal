#include "commands/download.h"
#include <sstream>

using namespace EmbeddedTerminal;
using namespace EmbeddedTerminal::cmd;
static const size_t RAW_CHUNK = 384;                     // 384 % 3 == 0
static const size_t B64_CHUNK = (RAW_CHUNK / 3) * 4 + 1; // +1 für Null-Terminator
// Base64-Tabelle
static const char b64_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

// Encodiere `inBuf[0..len-1]` (len % 3 == 0) nach Base64 in outBuf.
// outBuf muss mindestens (len/3*4+1) groß sein.
// Am Ende steht ein '\0'.
ETString base64encode(const unsigned char *data, size_t len)
{
    static const char TABLE[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    ETString out;
    out.reserve(((len + 2) / 3) * 4);

    size_t i = 0;
    // volle 3-Byte-Blöcke
    for (; i + 2 < len; i += 3)
    {
        unsigned long v = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
        out.push_back(TABLE[(v >> 18) & 0x3F]);
        out.push_back(TABLE[(v >> 12) & 0x3F]);
        out.push_back(TABLE[(v >> 6) & 0x3F]);
        out.push_back(TABLE[v & 0x3F]);
    }
    // Rest und Padding
    size_t rem = len - i;
    if (rem > 0)
    {
        unsigned long v = data[i] << 16;
        if (rem == 2)
            v |= data[i + 1] << 8;
        out.push_back(TABLE[(v >> 18) & 0x3F]);
        out.push_back(TABLE[(v >> 12) & 0x3F]);
        if (rem == 2)
            out.push_back(TABLE[(v >> 6) & 0x3F]);
        else
            out.push_back('=');
        out.push_back('=');
    }
    return out;
}

/// Erzeugt den Header "SIZE <fileSize>\n"
ETString createHeader(unsigned long fileSize)
{
    return "SIZE " + std::to_string(fileSize) + "\n";
}

ETString sendFile(ETFile &file)
{
    if (!file)
    {
        return createHeader(0) + "EOF\n";
    }

    size_t fileSize = file.size();

    ETString result = createHeader(fileSize);
    std::vector<unsigned char> buf(RAW_CHUNK);
    unsigned long sent = 0;
    while (sent < fileSize)
    {
        size_t toRead = RAW_CHUNK;
        if (fileSize - sent < RAW_CHUNK)
        {
            toRead = fileSize - sent;
        }
        size_t actuallyRead = file.read(buf.data(), toRead);
        if (actuallyRead == 0)
            break;
        ETString b64 = base64encode(buf.data(), actuallyRead);
        result += b64;
        result += "\n";
        sent += actuallyRead;
    }

    result += "EOF\n";
    return result;
}

ETString cmd::download::trigger(const ETString &keyword, const ETString &additional)
{

    additional.trim();
    auto param = split(additional, " ");

    if (param.size() != 1)
    {
        return usage(keyword);
    }

    auto file = _dir.open(param[0].trim(), FILE_MODE_READ, false);

    return sendFile(file);
}

ETString cmd::download::usage(const ETString &keyword)
{
    return "Download a specific file\n\n" +
           keyword + "[path] - Download the file under the given path\n If the file did not exists than just a filesize of zero will be returned";
}