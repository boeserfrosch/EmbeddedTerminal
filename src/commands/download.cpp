#include "commands/download.h"
#include "DefaultAutoCompleters.h"
#include <sstream>
#include "download.h"

using namespace EmbeddedTerminal;
using namespace EmbeddedTerminal::cmd;

static const size_t RAW_CHUNK = 384;                     // 384 % 3 == 0
static const size_t B64_CHUNK = (RAW_CHUNK / 3) * 4 + 1; // +1 für Null-Terminator

// Encodiere `inBuf[0..len-1]` (len % 3 == 0) nach Base64 in outBuf.
// outBuf muss mindestens (len/3*4+1) groß sein.
// Am Ende steht ein '\0'.
void base64encode(const unsigned char *data, size_t len, const CommandInvocation &invocation)
{
    static const char TABLE[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    size_t i = 0;
    // volle 3-Byte-Blöcke
    for (; i + 2 < len; i += 3)
    {
        unsigned long v = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
        invocation.streams.output.print(ETString() + TABLE[(v >> 18) & 0x3F] + TABLE[(v >> 12) & 0x3F] + TABLE[(v >> 6) & 0x3F] + TABLE[v & 0x3F]);
    }
    // Rest und Padding
    size_t rem = len - i;
    if (rem > 0)
    {
        unsigned long v = data[i] << 16;
        if (rem == 2)
        {
            v |= data[i + 1] << 8;
        }
        invocation.streams.output.print(ETString() + TABLE[(v >> 18) & 0x3F] + TABLE[(v >> 12) & 0x3F]);
        if (rem == 2)
        {
            invocation.streams.output.print(TABLE[(v >> 6) & 0x3F]);
        }
        else
        {
            invocation.streams.output.print('=');
        }
    }
}

CommandResult cmd::download::invoke(CommandInvocation &invocation)
{
    // First check íf we are resuming an existing download session, in this case we raise an error
    auto itPath = invocation.context.variables.find(ETString(SESSION_KEY_PATH));
    auto itPos = invocation.context.variables.find(ETString(SESSION_KEY_POS));
    if (itPath != invocation.context.variables.end() &&
        itPos != invocation.context.variables.end())
    {
        return error_(INVALID_INVOKE_EXPECTED_RESUME, invocation);
    }
    auto state = initState_(invocation);
    if (!state.initialized)
    {
        return error_(INVALID_INVOKE, invocation);
    }

    return processDownload(invocation, state);
}

CommandResult cmd::download::resume(CommandInvocation &invocation)
{
    auto state = getState_(invocation);
    if (!state.initialized)
    {
        return error_(INVALID_PATH, invocation);
    }
    return processDownload(invocation, state);
}

CommandResult cmd::download::processDownload(CommandInvocation &invocation, const State &state)
{
    ErrorCode code = checkState_(state, invocation);
    if (code != NONE)
    {
        return error_(code, invocation);
    }

    auto file = dir_.open(state.path, FILE_MODE_READ, false);
    if (!file.isOpen())
    {
        return error_(FAILED_TO_OPEN_FILE, invocation);
    }
    size_t fileSize = file.size();

    if (state.position == 0)
    {
        // First call, send initial header
        invocation.streams.output.print("SIZE " + toETString(fileSize) + "\n");
    }

    if (state.position >= fileSize)
    {
        invocation.streams.output.print("\nEOF\n");
        file.close();
        return success_(invocation);
    }

    if (!file.seek(state.position))
    {
        file.close();
        return error_(FAILED_TO_SEEK, invocation);
    }

    size_t numChunks = ((fileSize + RAW_CHUNK - 1) / RAW_CHUNK);
    size_t chunkIndex = state.position / RAW_CHUNK + 1;
    ETVector<unsigned char> buf(RAW_CHUNK);

    invocation.streams.output.print("CHUNK " + toETString(chunkIndex) + "/" + toETString(numChunks) + "\n");

    size_t actuallyRead = file.read(buf.data(), RAW_CHUNK);
    file.close();
    if (actuallyRead == 0)
    {
        // read error
        invocation.streams.output.print("\nEOF\n");
        return error_(FAILED_TO_READ, invocation);
    }

    base64encode(buf.data(), actuallyRead, invocation);
    invocation.context.variables[ETString(SESSION_KEY_POS)] = toETString(state.position + actuallyRead);

    if (state.position + actuallyRead >= fileSize)
    {
        invocation.streams.output.print("\nEOF\n");
        return success_(invocation);
    }

    return CommandResult::running(NONE);
}

ETString cmd::download::usage(const ETString &keyword) const
{
    return "Download a specific file\n\n" +
           keyword + " <path> - Download the file under the given path\n If the file did not exists than just a filesize of zero will be return ed";
}

ETVector<ETString> cmd::download::getSuggestions(const ETString &partial) const
{
    FilePathCompleter completer(dir_);
    return completer.getSuggestions(partial);
}

cmd::download::State EmbeddedTerminal::cmd::download::initState_(CommandInvocation &invocation)
{
    auto &vars = invocation.context.variables;

    State state;
    OptionParser parser;
    parser.addRequiredRemainingArgument("path");
    auto parseResult = parser.parse(invocation.arguments);
    if (!parseResult.success)
    {
        return state; // Will be handled as error in the caller
    }

    ETString path = parseResult.options["path"][0];

    // Store path for potential re-entry
    vars[download::SESSION_KEY_PATH] = path;
    vars[download::SESSION_KEY_POS] = "0";

    State result;
    result.path = Path(path);
    result.position = 0;
    result.initialized = true;
    return result;
}

cmd::download::State EmbeddedTerminal::cmd::download::getState_(CommandInvocation &invocation)
{
    auto &vars = invocation.context.variables;
    State state;
    if (vars.find(SESSION_KEY_PATH) == vars.end() || vars.find(SESSION_KEY_POS) == vars.end())
    {
        return state; // Will be handled as error in the caller
    }
    state.path = Path(vars[SESSION_KEY_PATH]);
    state.position = ETString::toull(vars[SESSION_KEY_POS].c_str());
    state.initialized = true;
    return state;
}

cmd::download::ErrorCode cmd::download::checkState_(const State &state, CommandInvocation &invocation)
{
    ErrorCode result = NONE;
    if (state.path.empty())
    {
        result = INVALID_PATH;
    }
    if (!dir_.exists(state.path.c_str()))
    {
        result = FILE_NOT_FOUND;
    }
    if (dir_.isDirectory(state.path.c_str()))
    {
        result = IS_DIRECTORY;
    }

    if (result != NONE)
    {
        reset(invocation);
    }
    return result;
}

CommandResult cmd::download::error_(size_t errorCode, CommandInvocation &invocation)
{
    reset(invocation);

    switch (errorCode)
    {
    case INVALID_PATH:
        invocation.streams.error.print("Expected parameter\n");
        break;
    case FILE_NOT_FOUND:
        invocation.streams.error.print("file did not exist\n");
        break;
    case IS_DIRECTORY:
        invocation.streams.error.print("file is a directory\n");
        break;
    case FAILED_TO_OPEN_FILE:
        invocation.streams.error.print("failed to open file\n");
        break;
    case FAILED_TO_SEEK:
        invocation.streams.error.print("failed to seek in file\n");
        break;
    case FAILED_TO_READ:
        invocation.streams.error.print("failed to read from file\n");
        break;
    case INVALID_INVOKE:
        invocation.streams.output.print(usage(invocation.keyword));
        break;
    case INVALID_INVOKE_EXPECTED_RESUME:
        invocation.streams.error.print("invalid invoke: expected resume call with existing session\n");
        break;
    default:
        invocation.streams.error.print("unknown error\n");
        break;
    }

    return CommandResult::completed(errorCode);
}

CommandResult cmd::download::success_(CommandInvocation &invocation)
{
    reset(invocation);
    return CommandResult::completed(NONE);
}

void EmbeddedTerminal::cmd::download::reset(CommandInvocation &invocation)
{
    invocation.context.variables.erase(SESSION_KEY_PATH);
    invocation.context.variables.erase(SESSION_KEY_POS);
}
