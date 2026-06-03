#include "interfaces/ICommandRuntime.h"
#include "commands/cat.h"
#include "cat.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {
        CommandResult cat::invoke(CommandInvocation &invocation)
        {
            OptionParser parser;
            parser.addRequiredRemainingArgument("file");
            auto parseResult = parser.parse(invocation.arguments);
            if (!parseResult.success)
            {
                invocation.streams.output.print("usage: " + usage(invocation.keyword));
                return CommandResult::completed(ErrorCode::INVALID_PATH);
            }

            invocation.context.variables[SESSION_KEY_PATH] = parseResult.options["file"][0];
            invocation.context.variables[SESSION_KEY_POS] = "0";

            return processExecution_(invocation);
        }

        CommandResult cat::processExecution_(CommandInvocation &invocation)
        {
            auto code = checkFilePath_(invocation);
            if (code != ErrorCode::NONE)
            {
                return CommandResult::completed(code);
            }
            ETString path = invocation.context.variables[SESSION_KEY_PATH];
            size_t filePos = ETString::toull(invocation.context.variables[SESSION_KEY_POS].c_str());

            auto file = dir_.open(path.c_str(), "r", false);
            if (!file.isOpen())
            {
                reset(invocation);
                invocation.streams.error.print("failed to open file\n");
                return CommandResult::completed(ErrorCode::FAILED_TO_OPEN_FILE);
            }

            if (!file.seek(filePos))
            {
                file.close();
                reset(invocation);
                invocation.streams.error.print("failed to seek file\n");
                return CommandResult::completed(ErrorCode::FAILED_TO_READ);
            }

            // Read and emit one chunk (512 bytes max)
            unsigned char buffer[CHUNK_SIZE];
            size_t bytesRead = file.read(buffer, CHUNK_SIZE);

            if (bytesRead <= 0)
            {
                file.close();
                reset(invocation);
                invocation.streams.error.print("failed to read file\n");
                return CommandResult::completed(ErrorCode::FAILED_TO_READ);
            }

            ETString chunk(std::string(reinterpret_cast<const char *>(buffer), bytesRead));
            invocation.streams.output.print(chunk);

            size_t newPos = filePos + bytesRead;

            // Update position for next chunk
            invocation.context.variables[SESSION_KEY_POS] = toETString(newPos);

            file.close();
            if (bytesRead == CHUNK_SIZE && newPos < file.size())
            {
                return CommandResult::running(ErrorCode::NONE); // More data to read, keep running
            }
            else
            {
                reset(invocation);
                return CommandResult::completed(ErrorCode::NONE); // Completed reading the file
            }
        }

        CommandResult cat::resume(CommandInvocation &invocation)
        {

            return processExecution_(invocation);
        }

        ETString cat::usage(const ETString &keyword) const
        {
            return keyword + " <file> - Returns the content of the defined file\n";
        }

        ETVector<ETString> cat::getSuggestions(const ETString &partial) const
        {
            // Delegate to FilePathCompleter
            return completer_.getSuggestions(partial);
        }

        cat::ErrorCode cat::checkFilePath_(CommandInvocation &invocation)
        {
            ETString path = invocation.context.variables[SESSION_KEY_PATH];
            if (path.empty())
            {
                invocation.streams.error.print("path or name to file expected\n");
                return ErrorCode::INVALID_PATH; // No file specified
            }
            if (!dir_.exists(path.c_str()))
            {
                invocation.streams.error.print("file " + path + " did not exist\n");
                return ErrorCode::FILE_NOT_FOUND; // File does not exist
            }
            if (dir_.isDirectory(path.c_str()))
            {
                invocation.streams.error.print("file " + path + " is a directory\n");
                return ErrorCode::IS_DIRECTORY; // File does not exist or is a directory
            }
            return ErrorCode::NONE; // State is valid
        }

        void cat::reset(CommandInvocation &invocation)
        {
            invocation.context.variables.erase(SESSION_KEY_PATH);
            invocation.context.variables.erase(SESSION_KEY_POS);
        }
    }
}
