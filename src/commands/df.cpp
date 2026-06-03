#include "commands/df.h"
#include "df.h"

using namespace EmbeddedTerminal::cmd;

ETString df::usage(const ETString &keyword) const
{
    return keyword + " - Show disk usage \n";
}

EmbeddedTerminal::CommandResult EmbeddedTerminal::cmd::df::invoke(CommandInvocation &invocation)
{
    OptionParser parser;
    parser.addOptionalRemainingArgument("path");
    auto parseResult = parser.parse(invocation.arguments);
    if (!parseResult.success)
    {
        invocation.streams.output.print(usage(invocation.keyword));
        return CommandResult::completed(1); // Error code for invalid arguments
    }
    ETString path = parseResult.remainingArguments.empty() ? ETString() : parseResult.remainingArguments[0];

    const float dim = 1024.0f * 1024.0f;
    ETVector<IStorageMedia *> medias;
    if (path.empty())
    {
        medias = storage_.media();
    }
    else
    {
        auto media = storage_.getMediaFromPath(path);
        if (!media)
        {
            invocation.streams.output.print("Storage media not found\n");
            return CommandResult::completed(ErrorCode::FailedToRetrieveMedia); // Error code for storage media not found
        }
        medias.push_back(media);
    }

    invocation.streams.output.print("Filesystem\tSize\tUsed\tFree\n");
    for (auto *media : medias)
    {
        const float sizeMb = static_cast<float>(media->totalBytes()) / dim;
        const float usedMb = static_cast<float>(media->usedBytes()) / dim;
        const float freeMb = static_cast<float>(media->freeBytes()) / dim;

        invocation.streams.output.print(media->name());
        invocation.streams.output.print("\tSize ");
        invocation.streams.output.print(toETString(static_cast<size_t>(sizeMb)));
        invocation.streams.output.print(".0 MB");
        invocation.streams.output.print("\tUsed ");
        invocation.streams.output.print(toETString(static_cast<size_t>(usedMb)));
        invocation.streams.output.print(".0 MB");
        invocation.streams.output.print("\tFree ");
        invocation.streams.output.print(toETString(static_cast<size_t>(freeMb)));
        invocation.streams.output.print(".0 MB\n");
    }

    return CommandResult::completed(ErrorCode::None);
}
