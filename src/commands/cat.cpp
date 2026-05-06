#include "interfaces/ICommandRuntime.h"
#include "commands/cat.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {
        ETString cat::readFileForTrigger(const ETString &additional)
        {
            OptionParser parser;
            parser.addRequiredRemainingArgument("file");
            auto parseResult = parser.parse(additional);
            if (!parseResult.success)
            {
                return parseResult.errorMessage + "\n" + usage("cat");
            }
            ETString path = parseResult.options["file"][0];

            if (!dir_.exists(path.c_str()) || dir_.isDirectory(path.c_str()))
            {
                return "file " + path + " did not exist!\n";
            }
            auto file = dir_.open(path.c_str(), "r", false);
            if (file.size() < 512)
            {
                auto content = file.readAll();
                file.close();
                return ETString(content.c_str()) + "\n";
            }
            unsigned char buffer[512];
            file.read(buffer, 512);
            file.close();

            ETString chunk = buffer;
            return chunk + "\n" + "... File truncated ...";
        }

        CommandResult cat::executeStream(CommandInvocation &invocation)
        {
            CatState state = handleState_(invocation);
            auto code = checkState_(state, invocation);
            if (code != errorCodes::CAT_CMD_ERROR_NONE)
            {
                return CommandResult::completed(code);
            }

            ETString path = state.path;
            size_t filePos = state.position;

            auto file = dir_.open(path.c_str(), "r", false);
            if (!file.isOpen())
            {
                invocation.context.variables.erase(SESSION_KEY_PATH);
                invocation.context.variables.erase(SESSION_KEY_POS);
                invocation.stderrChannel.print("failed to open file\n");
                return CommandResult::completed(errorCodes::CAT_CMD_ERROR_FAILED_TO_OPEN_FILE);
            }

            // Seek to current position
            file.seek(filePos);

            bool hasMore = processChunk_(file, filePos, invocation);

            file.close();
            if (hasMore)
            {
                return CommandResult::running(errorCodes::CAT_CMD_ERROR_NONE);
            }
            else
            {
                invocation.context.variables.erase(SESSION_KEY_PATH);
                invocation.context.variables.erase(SESSION_KEY_POS);
                return CommandResult::completed(errorCodes::CAT_CMD_ERROR_NONE);
            }
        }

        CommandResult cat::execute(CommandInvocation &invocation)
        {
            return executeStream(invocation);
        }

        ETString cat::trigger(const ETString &keyword, const ETString &additional)
        {
            // Deprecated trigger, only used for non-streaming usage of cat
            (void)keyword;
            return readFileForTrigger(additional);
        }

        ETString cat::usage(const ETString &keyword)
        {
            return keyword + " <file> - Returns the content of the defined file\n";
        }

        ETVector<ETString> cat::getSuggestions(const ETString &partial)
        {
            // Delegate to FilePathCompleter
            return completer_.getSuggestions(partial);
        }

        EmbeddedTerminal::cmd::cat::CatState cat::handleState_(CommandInvocation &invocation)
        {
            // Check if this is a continuation of an ongoing stream
            auto &vars = invocation.context.variables;
            auto pathIt = vars.find(SESSION_KEY_PATH);
            auto posIt = vars.find(SESSION_KEY_POS);

            ETString path;
            size_t filePos = 0;

            if (pathIt != vars.end())
            {
                // Continued stream
                path = pathIt->second;
                posIt = vars.find(SESSION_KEY_POS);
                if (posIt != vars.end())
                {
                    filePos = std::stoull(posIt->second.c_str());
                }
            }
            else
            {
                OptionParser parser;
                parser.addRequiredRemainingArgument("file");
                auto parseResult = parser.parse(invocation.arguments);
                if (!parseResult.success)
                {
                    return CatState(); // Will be handled as error in the caller
                }
                // New stream request
                path = parseResult.options["file"][0];

                // Store path for potential re-entry
                vars[SESSION_KEY_PATH] = path;
                vars[SESSION_KEY_POS] = "0";
            }

            CatState state;
            state.path = path;
            state.position = filePos;
            return state;
        }

        errorCodes::CatCmdErrorCode cat::checkState_(const CatState &state, CommandInvocation &invocation)
        {
            if (state.path.empty())
            {
                invocation.stderrChannel.print("path or name to file expected\n");
                return errorCodes::CAT_CMD_ERROR_INVALID_PATH; // No file specified
            }
            if (!dir_.exists(state.path.c_str()))
            {
                invocation.stderrChannel.print("file " + state.path + " did not exist\n");
                return errorCodes::CAT_CMD_ERROR_FILE_NOT_FOUND; // File does not exist
            }
            if (dir_.isDirectory(state.path.c_str()))
            {
                invocation.stderrChannel.print("file " + state.path + " is a directory\n");
                return errorCodes::CAT_CMD_ERROR_IS_DIRECTORY; // File does not exist or is a directory
            }
            return errorCodes::CAT_CMD_ERROR_NONE; // State is valid
        }

        bool cat::processChunk_(ETFile &file, size_t filePos, CommandInvocation &invocation)
        {
            // Read and emit one chunk (512 bytes max)
            unsigned char buffer[CHUNK_SIZE];
            size_t bytesRead = file.read(buffer, CHUNK_SIZE);

            if (bytesRead > 0)
            {
                ETString chunk(std::string(reinterpret_cast<const char *>(buffer), bytesRead));
                invocation.stdoutChannel.print(chunk);

                size_t newPos = filePos + bytesRead;

                // Update position for next chunk
                invocation.context.variables[SESSION_KEY_POS] = toETString(newPos);

                return bytesRead == CHUNK_SIZE; // Return true if there might be more data
            }
            else
            {
                return false; // No more data to read
            }
        }
    }
}
