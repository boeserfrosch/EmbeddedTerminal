#include "commands/df.h"

using namespace EmbeddedTerminal::cmd;
ETString df::trigger(ETString &keyword, ETString &additional)
{
    char sizeStr[256];
    auto dim = 1024 * 1024.0;
    auto dimStr = "MB";
    auto cSize = (float)(_card.capacity() / dim);
    auto totalBytes = (float)(_card.totalBytes() / dim);
    auto usedBytes = (float)(_card.usedBytes() / dim);
    auto freeBytes = (float)(totalBytes - usedBytes);
    snprintf(sizeStr, 256, "SD Card: \tSize %.1f %s, \tTotal %.1f %s, \tUsed: %.1f %s (Free: %3.1f %s )\n", cSize, dimStr, totalBytes, dimStr, usedBytes, dimStr, (freeBytes / totalBytes) * 100, "%");

    return sizeStr;
}

ETString df::usage(ETString &keyword)
{
    return keyword + " - Show disk usage \n";
}
