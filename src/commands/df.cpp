#include "commands/df.h"

using namespace EmbeddedTerminal::cmd;
ETString df::trigger(const ETString &keyword, const ETString &additional)
{
    const float dim = 1024.0f * 1024.0f;
    ETString result = "Filesystem\tSize\tUsed\tFree\n";

    ETVector<IStorageMedia *> medias;
    ETString path = additional.trim();
    if (path.empty())
    {
        medias = storage_.media();
    }
    else
    {
        auto media = storage_.getMediaFromPath(path);
        if (!media)
        {
            return "Storage media not found\n";
        }
        medias.push_back(media);
    }

    for (auto *media : medias)
    {
        const float sizeMb = static_cast<float>(media->totalBytes()) / dim;
        const float usedMb = static_cast<float>(media->usedBytes()) / dim;
        const float freeMb = static_cast<float>(media->freeBytes()) / dim;

        result += media->name();
        result += "\tSize ";
        result += toETString(static_cast<size_t>(sizeMb));
        result += ".0 MB";
        result += "\tUsed ";
        result += toETString(static_cast<size_t>(usedMb));
        result += ".0 MB";
        result += "\tFree ";
        result += toETString(static_cast<size_t>(freeMb));
        result += ".0 MB\n";
    }

    return result;
}

ETString df::usage(const ETString &keyword)
{
    return keyword + " - Show disk usage \n";
}
