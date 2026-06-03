#include "script.h"
#include "OptionParser.h"
#include "lang/LangAPI.h"
#include "scripting/Runner.h"

using namespace EmbeddedTerminal;

namespace EmbeddedTerminal::cmd
{

    ETString script::usage(const ETString &keyword) const
    {
        return keyword + " <path> - Execute script from file\n";
    }

    CommandResult script::invoke(CommandInvocation &invocation)
    {
        if (runner_.isInitialized())
        {
            invocation.streams.error.print("A script is already running. Please wait for it to finish or interrupt it before starting a new one.\n");
            return CommandResult::completed(ErrorCode::RuntimeError);
        }

        tryFetchAndParseScript(invocation);
        if (error())
        {
            return CommandResult::completed(error_);
        }

        return executeRunner_(invocation);
    }
    CommandResult script::executeRunner_(CommandInvocation &invocation)
    {
        EmbeddedTerminal::Scripting::Runner::State state = runner_.execute(scriptAst_, runnerVariables_);

        if (runner_.error())
        {
            invocation.streams.error.print("script error: runtime error\n");
            reset();
            return CommandResult::completed(ErrorCode::RuntimeError);
        }

        if (state == EmbeddedTerminal::Scripting::Runner::State::Completed)
        {
            reset();
            return CommandResult::completed(0);
        }

        // Runner reported running; map last pause state to waiting/running
        if (runner_.getLastPauseState() == CommandExecutionState::WaitingForInput)
        {
            return CommandResult::waitingForInput(0);
        }

        return CommandResult::running(0);
    }

    CommandResult script::resume(CommandInvocation &invocation)
    {
        if (!runner_.isInitialized())
        {
            invocation.streams.error.print("No script is currently running\n");
            return CommandResult::completed(ErrorCode::RuntimeError);
        }

        return executeRunner_(invocation);
    }

    void script::onInterrupt()
    {
        if (runner_.isInitialized())
        {
            runner_.interrupt();
            reset();
        }
    }

    ETString script::getErrorMessage_(ErrorCode errorCode) const
    {
        switch (errorCode)
        {
        case ErrorCode::None:
            return "No error";
        case ErrorCode::InvalidArguments:
            return "Invalid arguments";
        case ErrorCode::FileNotFound:
            return "File not found";
        case ErrorCode::FilesystemNotAvailable:
            return "Filesystem not available";
        case ErrorCode::FileError:
            return "File error";
        case ErrorCode::ParseError:
            return "Parse error";
        case ErrorCode::RuntimeError:
            return "Runtime error";
        default:
            return "Unknown error";
        }
    }

    void script::tryFetchAndParseScript(CommandInvocation &invocation)
    {
        ETString scriptPath = tryFetchScriptPathFromArguments(invocation);
        if (error())
        {
            return;
        }

        tryFetchScriptFromPath(scriptPath, invocation);
        if (error())
        {
            return;
        }

        tryTokenizeAndParseScript(invocation);
        if (error())
        {
            return;
        }

        // Persist invocation variables so runner can resume without restarting
        runnerVariables_ = invocation.context.variables;
        scriptLoaded_ = true;
        return;
    }

    void script::tryTokenizeAndParseScript(CommandInvocation &invocation)
    {
        EmbeddedTerminal::Lexer lexer;
        token_list_t tokens = lexer.tokenize(scriptContent_);
        for (const auto &token : tokens)
        {
            if ((token.type == TokenType::DOUBLE_QUOTE_STRING_LITERAL || token.type == TokenType::SINGLE_QUOTE_STRING_LITERAL) && !token.terminated)
            {
                invocation.streams.error.print("script error: unterminated quote\n");
                scriptContent_ = "";
                error_ = ErrorCode::ParseError;
                return;
            }
        }

        EmbeddedTerminal::Scripting::ScriptParser scriptParser;
        scriptAst_ = scriptParser.parse(tokens);
        if (scriptParser.error())
        {
            invocation.streams.error.print("script error: parse error\n");
            scriptContent_ = "";
            error_ = ErrorCode::ParseError;
            return;
        }
    }

    ETString script::tryFetchScriptPathFromArguments(CommandInvocation &invocation)
    {
        OptionParser parser;
        parser.addRequiredRemainingArgument("file");
        auto parseResult = parser.parse(invocation.arguments);
        if (!parseResult.success)
        {
            invocation.streams.error.print(usage(invocation.keyword));
            error_ = ErrorCode::InvalidArguments;
            return "";
        }
        return parseResult.options["file"][0];
    }

    void script::tryFetchScriptFromPath(const ETString &path, CommandInvocation &invocation)
    {
        IFileSystem *fileSystem = terminal_.getFileSystem();
        if (fileSystem == nullptr)
        {
            invocation.streams.error.print("script error: File system not available\n");
            error_ = ErrorCode::FilesystemNotAvailable;
            return;
        }

        if (!fileSystem->exists(path))
        {
            invocation.streams.error.print("script error: File not found: " + path + "\n");
            error_ = ErrorCode::FileNotFound;
            return;
        }

        auto file = fileSystem->open(path, FILE_MODE_READ, false);
        if (!file || !file.isOpen())
        {
            invocation.streams.error.print("script error: Failed to open file: " + path + "\n");
            error_ = ErrorCode::FileError;
            return;
        }

        scriptContent_ = file.readAll();
        file.close();
    }

    void script::reset()
    {
        scriptContent_ = "";
        scriptAst_ = EmbeddedTerminal::Scripting::ExpressionChain();
        runnerVariables_.clear();
        runner_.reset();
        scriptLoaded_ = false;
    }
}