#include "interfaces/ICommandRuntime.h"
#include "commands/cat.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {
        ETString cat::readFileForTrigger(const ETString &additional)
        {
            ETString path = additional.trim();
            if (path.empty())
            {
                return "path or name to file expected\n";
            }
            if (!_dir.exists(path.c_str()) || _dir.isDirectory(path.c_str()))
            {
                return "file " + path + " did not exist!\n";
            }
            auto file = _dir.open(path.c_str(), "r", false);
            if (file.size() < 512)
            {
                auto content = file.readAll();
                file.close();
                return ETString(content.c_str()) + "\n";
            }
            unsigned char buffer[512];
            size_t bytesRead = file.read(buffer, 512);
            file.close();

            ETString chunk = buffer;
            return chunk + "\n" + "... File truncated ...";
        }

        CommandResult cat::executeStream(CommandInvocation &invocation)
        {

            CatState state = _handleState(invocation);
            unsigned char code = _checkState(state, invocation);
            if (code != 0)
            {
                return CommandResult::completed(code);
            }

            ETString path = state.path;
            size_t filePos = state.position;

            auto file = _dir.open(path.c_str(), "r", false);
            if (!file.isOpen())
            {
                invocation.context.variables.erase(SESSION_KEY_PATH);
                invocation.context.variables.erase(SESSION_KEY_POS);
                invocation.stderrChannel.print("failed to open file\n");
                return CommandResult::completed(2);
            }

            // Seek to current position
            file.seek(filePos);

            bool hasMore = _processChunk(file, filePos, invocation);

            file.close();
            if (hasMore)
            {
                return CommandResult::running(0);
            }
            else
            {
                invocation.context.variables.erase(SESSION_KEY_PATH);
                invocation.context.variables.erase(SESSION_KEY_POS);
                return CommandResult::completed(0);
            }
        }

        CommandResult cat::execute(CommandInvocation &invocation)
        {
            return executeStream(invocation);
        }

        ETString cat::trigger(const ETString &keyword, const ETString &additional)
        {
            (void)keyword;
            return readFileForTrigger(additional);
        }

        ETString cat::usage(const ETString &keyword)
        {
            return keyword + " [file] - Returns the content of the defined file (at max the first 512 bytes)\n";
        }

        ETVector<ETString> cat::getSuggestions(const ETString &partial)
        {
            // Delegate to FilePathCompleter
            return _completer.getSuggestions(partial);
        }

        EmbeddedTerminal::cmd::cat::CatState cat::_handleState(CommandInvocation &invocation)
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
                // New stream request
                path = invocation.arguments.trim();

                // Store path for potential re-entry
                vars[SESSION_KEY_PATH] = path;
                vars[SESSION_KEY_POS] = "0";
            }

            CatState state;
            state.path = path;
            state.position = filePos;
            return state;
        }

        unsigned char cat::_checkState(const CatState &state, CommandInvocation &invocation)
        {
            if (state.path.empty())
            {
                invocation.stderrChannel.print("path or name to file expected\n");
                return 1; // No file specified
            }
            if (!_dir.exists(state.path.c_str()))
            {
                invocation.stderrChannel.print("file did not exist\n");
                return 2; // File does not exist
            }
            if (_dir.isDirectory(state.path.c_str()))
            {
                invocation.stderrChannel.print("file is a directory\n");
                return 2; // File does not exist or is a directory
            }
            return 0; // State is valid
        }

        bool cat::_processChunk(ETFile &file, size_t filePos, CommandInvocation &invocation)
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
