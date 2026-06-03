#include "Runner.h"
#include "lang/LangAPI.h"
#include "FeatureFlags.h"
#include "channels/FileInput.h"
#include "channels/FileOutput.h"
#include "channels/BufferedInput.h"
#include "channels/BufferedOutput.h"

using namespace EmbeddedTerminal::Lang;

namespace EmbeddedTerminal
{
    namespace Scripting
    {

        bool evalStringToBool(const ETString &s)
        {
            return !s.empty() && s != "false" && s != "0";
        }

        void Runner::resetExecutionState_()
        {
            currentAst_ = nullptr;
            currentIndex_ = 0;
            currentVariables_ = nullptr;
            lastCommandExitCode_ = 0;
            callStack_.clear();
        }

        bool Runner::setContextInputChannelForRedirects(const CommandExpression &commandExpr, ETMap<ETString, ETString> &variables, std::unique_ptr<IInputChannel> &ownedInputChannel)
        {
            if (!commandExpr.hasRedirectIn)
            {
                return true;
            }
            ETString redirectInPath;
            if (!resolveTokenValue(commandExpr.redirectInPath, variables, redirectInPath))
            {
                return false;
            }
            auto *fs = ctx_.getFileSystem();
            if (fs == nullptr)
            {
                error_ = ErrorCode::RuntimeError;
                return false;
            }
            ETFile f = fs->open(Path(redirectInPath), FILE_MODE_READ, false);
            if (!f.isOpen())
            {
                error_ = ErrorCode::RuntimeError;
                return false;
            }
            ownedInputChannel = std::make_unique<Channels::FileInput>(f);
            ctx_.setInputStream(ownedInputChannel.get());
            return true;
        }

        bool Runner::setContextOutputChannelForRedirects(const CommandExpression &commandExpr, ETMap<ETString, ETString> &variables, std::unique_ptr<IOutputChannel> &ownedOutputChannel)
        {
            if (!commandExpr.hasRedirectOut)
            {
                return true;
            }
            ETString redirectOutPath;
            if (!resolveTokenValue(commandExpr.redirectOutPath, variables, redirectOutPath))
            {
                return false;
            }
            auto *fs = ctx_.getFileSystem();
            if (fs == nullptr)
            {
                error_ = ErrorCode::RuntimeError;
                return false;
            }
            ETFile f = fs->open(Path(redirectOutPath), commandExpr.appendRedirect ? FILE_MODE_APPEND : FILE_MODE_WRITE, true);
            if (!f.isOpen())
            {
                error_ = ErrorCode::RuntimeError;
                return false;
            }
            ownedOutputChannel = std::make_unique<Channels::FileOutput>(f);
            ctx_.setOutputStream(ownedOutputChannel.get());
            return true;
        }

        bool Runner::setContextErrorChannelForRedirects(const CommandExpression &commandExpr, ETMap<ETString, ETString> &variables)
        {
            // Not yet supported to redirect error stream separately from output stream, but we can implement this in the future if needed by adding another redirectOutErrorPath to CommandExpression and handling it here
            return true;
        }

        bool Runner::resolveKeywordAndArguments(const CommandExpression &commandExpr, ETMap<ETString, ETString> &variables, ETString &keyword, ETVector<ETString> &arguments)
        {
            if (!resolveTokenValue(commandExpr.keyword, variables, keyword))
            {
                return false;
            }
            for (const auto &argToken : commandExpr.arguments)
            {
                ETString argValue;
                if (!resolveTokenValue(argToken, variables, argValue))
                {
                    return false;
                }
                arguments.push_back(argValue);
            }
            return true;
        }

        bool Runner::updateCommandExecutionState(CommandResult result)
        {
            if (commandStackTop_ == 0)
            {
                error_ = ErrorCode::RuntimeError;
                return false;
            }

            auto stackTop = commandStackTop();
            if (stackTop == nullptr || stackTop->invocation == nullptr)
            {
                error_ = ErrorCode::RuntimeError;
                return false;
            }

            lastCommandExitCode_ = result.exitCode;
            stackTop->invocation->context.lastExitCode = result.exitCode;
            stackTop->invocation->context.variables["?"] = result.exitCode;

            switch (result.state)
            {
            case CommandExecutionState::Completed:
                stackTop->invocation.reset();
                stackTop->command = nullptr;
                stackTop->state = CommandExecutionState::Completed;
                commandStackTop_--;
                return true;
            case CommandExecutionState::Running:
                stackTop->state = CommandExecutionState::Running;
                return false;
            case CommandExecutionState::WaitingForInput:
                stackTop->state = CommandExecutionState::WaitingForInput;
                return false;
            default:
                error_ = ErrorCode::RuntimeError;
                return false;
            }
        }

        Runner::StackNode *Runner::commandStackTop()
        {
            if (commandStackTop_ == 0)
            {
                return nullptr;
            }
            return &commandStack_[commandStackTop_ - 1];
        }

        bool Runner::resolveTokenValue(const token_t &token, ETMap<ETString, ETString> &variables, ETString &out)
        {
            if (isReservedWordToken(token.type))
            {
                out = token.text;
                return true;
            }

            switch (token.type)
            {
            case TokenType::WORD:
            case TokenType::SINGLE_QUOTE_STRING_LITERAL:
            case TokenType::TRUE:
            case TokenType::FALSE:
            case TokenType::EQUALS:
                out = token.text;
                return true;
            case TokenType::DOUBLE_QUOTE_STRING_LITERAL:
            {
                // Handle interpolation inside double-quoted string literals
                if (token.terminated && token.text.find('$') != ETString::npos)
                {
                    Lexer lexer;
                    ETString tokenText = token.text;
                    token_list_t innerTokens = lexer.tokenize(tokenText);
                    ETString result = tokenText;
                    for (size_t i = innerTokens.size(); i-- > 0;)
                    {
                        token_t &innerToken = innerTokens[i];
                        if (innerToken.type == TokenType::VARIABLE)
                        {
                            auto it = variables.find(innerToken.text);
                            if (it == variables.end())
                            {
                                error_ = ErrorCode::UndefinedVariable;
                                return false;
                            }
                            result.replace(innerToken.startIndex, innerToken.endIndex - innerToken.startIndex, it->second);
                        }
                    }
                    out = result;
                    return true;
                }
                out = token.text;
                return true;
            }
            case TokenType::VARIABLE:
            {
                auto it = variables.find(token.text);
                if (it == variables.end())
                {
                    error_ = ErrorCode::UndefinedVariable;
                    return false;
                }
                out = it->second;
                return true;
            }
            default:
                return false;
            }
        }

        bool Runner::evalTokenToBool(const token_t &token, ETMap<ETString, ETString> &variables)
        {
            ETString text;
            if (token.type == TokenType::TRUE || token.type == TokenType::FALSE)
            {
                return token.type == TokenType::TRUE;
            }
            if (token.type == TokenType::VARIABLE || token.type == TokenType::DOUBLE_QUOTE_STRING_LITERAL)
            {
                if (!resolveTokenValue(token, variables, text))
                {
                    return false;
                }
            }
            else
            {
                text = token.text;
            }
            return evalStringToBool(text);
        }

        bool Runner::executeChain_(const ExpressionChain &ast, ETMap<ETString, ETString> &variables)
        {
            // If entering a nested AST (function body or block), push the current context
            bool pushed = false;
            {
                CallFrame frame;
                frame.ast = currentAst_;
                frame.index = currentIndex_;
                frame.previousVariables = currentVariables_;
                // If the variables for the new AST are different from the current variables,
                // copy them into the frame so they remain valid across pauses.
                if (currentVariables_ != &variables)
                {
                    frame.ownedVariables = variables;
                    frame.ownsVariables = true;
                    currentVariables_ = &frame.ownedVariables;
                }
                callStack_.push_back(std::move(frame));
                currentAst_ = &ast;
                currentIndex_ = 0;
                pushed = true;
            }

            // Execute starting from currentIndex_ so that execution can resume where it left off
            while (currentIndex_ < ast.size())
            {
                const Expression &expression = ast[currentIndex_];
                if (!executeExpression_(expression, variables))
                {
                    // If an error occurred or we paused, leave currentAst_/currentIndex_ as-is for resume
                    return false;
                }
                // Advance to next expression
                if (currentVariables_ != &variables)
                {
                    currentVariables_ = &callStack_.back().ownedVariables;
                }
                ++currentIndex_;
            }

            // Finished this AST; restore previous context if any
            if (pushed)
            {
                CallFrame frame = callStack_.back();
                callStack_.pop_back();
                currentAst_ = frame.ast;
                currentIndex_ = frame.index;
                currentVariables_ = frame.previousVariables;
            }

            return true;
        }

        bool Runner::executeExpression_(const Expression &expression, ETMap<ETString, ETString> &variables)
        {
            switch (expression.tag)
            {
            case ExpressionType::VariableAssignment:
                return executeVariableAssignment(expression.data.variableAssignment, variables);
            case ExpressionType::Pipeline:
                return executePipelineExpression(expression.data.pipeline, variables);
            case ExpressionType::Command:
                return executeCommandExpression(expression.data.command, variables);
            case ExpressionType::If:
#if ET_ENABLE_SCRIPT_IF
                return executeIfExpression(expression.data.ifExpr, variables);
#else
                error_ = ErrorCode::SyntaxError;
                return false;
#endif
            case ExpressionType::ForLoop:
#if ET_ENABLE_SCRIPT_FOR
                return executeForLoop(expression.data.forLoop, variables);
#else
                error_ = ErrorCode::SyntaxError;
                return false;
#endif
            case ExpressionType::WhileLoop:
#if ET_ENABLE_SCRIPT_WHILE
                return executeWhileLoop(expression.data.whileLoop, variables);
#else
                error_ = ErrorCode::SyntaxError;
                return false;
#endif
            case ExpressionType::FunctionDefinition:
#if ET_ENABLE_SCRIPT_FUNCTIONS
                return executeFunctionDefinition(expression.data.functionDef, variables);
#else
                error_ = ErrorCode::SyntaxError;
                return false;
#endif
            case ExpressionType::FunctionCall:
#if ET_ENABLE_SCRIPT_FUNCTIONS
                return executeFunctionCall(expression.data.functionCall, variables);
#else
                error_ = ErrorCode::SyntaxError;
                return false;
#endif
            case ExpressionType::Nil:
            default:
                error_ = ErrorCode::SyntaxError;
                return false;
            }
        }

        void Runner::reset()
        {
            // Reset the runner's state so that it can be used for a fresh execution. This is useful for the 'script' command to reset the runner when a new script is loaded, and also for testing.
            resetExecutionState_();
            error_ = ErrorCode::None;
            // Functions are stored in the execution context; reset of Runner does not clear them.
            callStack_.clear();
            currentAst_ = nullptr;
            currentIndex_ = 0;
            currentVariables_ = nullptr;
        }

        Runner::State Runner::execute(const ExpressionChain &ast, ETMap<ETString, ETString> &variables)
        {
            // Determine whether this execute() call is initializing a fresh run
            bool freshRun = false;
            if (currentAst_ != &ast)
            {
                currentAst_ = &ast;
                currentVariables_ = &variables;
                currentIndex_ = 0;
                error_ = ErrorCode::None;
                freshRun = true;
            }

            if (error_ != ErrorCode::None)
            {
                resetExecutionState_();
                return State::Error;
            }

            if (currentAst_ == nullptr)
            {
                return State::Completed;
            }

            // If this is a fresh execute call (freshRun), run the entire chain to completion.
            // Do NOT treat a resume call (even if index==0) as a fresh run.
            if (freshRun)
            {
                bool ok = executeChain_(ast, variables);
                if (!ok)
                {
                    if (error_ != ErrorCode::None)
                    {
                        resetExecutionState_();
                        return State::Error;
                    }
                    if (isPaused_())
                    {
                        return State::Running;
                    }
                    resetExecutionState_();
                    return State::Completed;
                }
                resetExecutionState_();
                return State::Completed;
            }

            if (currentIndex_ >= currentAst_->size())
            {
                resetExecutionState_();
                return State::Completed;
            }

            // If we are paused waiting for input, do not re-invoke the command until input is available.
            bool resumedWaitingCommandThisCall = false;
            if (isPaused_())
            {
                const StackNode *stackTop = commandStackTop();
                if (stackTop != nullptr && stackTop->state == CommandExecutionState::WaitingForInput && !ctx_.getInputStream()->available())
                {
                    return State::Running;
                }
                if (stackTop != nullptr && stackTop->state == CommandExecutionState::WaitingForInput)
                {
                    // If input is available, allow execution to proceed; executeExpression_ will resume the command via Terminal::resumeCommandForScript
                    resumedWaitingCommandThisCall = true;
                }
            }

            const Expression &expression = (*currentAst_)[currentIndex_];
            if (!executeExpression_(expression, variables))
            {
                if (error_ != ErrorCode::None)
                {
                    resetExecutionState_();
                    return State::Error;
                }
                if (isPaused_())
                {
                    return State::Running;
                }
                resetExecutionState_();
                return State::Completed;
            }

            ++currentIndex_;
            if (resumedWaitingCommandThisCall)
            {
                // After resuming a waiting command, yield back to the caller so the script can be stepped
                // on the next execute() call.
                // IN the former if the command was executed so we should return the actual state after executing the command, which could be Running if the command is still running after the resume, or WaitingForInput if it hit another input wait, or Completed if it completed during the resume.
                const StackNode *stackTop = commandStackTop();
                if (stackTop != nullptr)
                {
                    switch (stackTop->state)
                    {
                    case CommandExecutionState::Running:
                        return State::Running;
                    case CommandExecutionState::WaitingForInput:
                        return State::Running;
                    case CommandExecutionState::Completed:
                        // Advance to next expression after a completed command
                        ++currentIndex_;
                        break;
                    default:
                        break;
                    }
                }
            }
            if (currentIndex_ >= currentAst_->size())
            {
                resetExecutionState_();
                return State::Completed;
            }

            return State::Running;
        }

        void Runner::interrupt()
        {
            // Clear the paused/running state so that any currently paused command will not be resumed
            // Send interrupt to the currently running command in the terminal, if any, via public API
            ctx_.interruptActiveCommand();
            // Reset execution state to clear any nested calls and return to a clean state
            resetExecutionState_();
        }

        bool Runner::executeCommandExpression(const CommandExpression &commandExpr, ETMap<ETString, ETString> &variables)
        {
            // If out stack is too deep, we have to error out to prevent overflow. This can happen if there is infinite recursion in the script, for example via a function that calls itself without a base case.
            if (commandStackTop_ >= MaxCommandStackDepth)
            {
                error_ = ErrorCode::StackOverflow;
                return false;
            }

            // A command expression is a simple command with arguments, without any pipes and optionally with redirects
            ETString keyword;
            ETVector<ETString> arguments;

            if (!resolveKeywordAndArguments(commandExpr, variables, keyword, arguments))
            {
                return false;
            }

            if (!ctx_.existsCommand(keyword))
            {
                error_ = ErrorCode::CommandNotFound;
                return false;
            }

            IInputChannel *savedInput = ctx_.getInputStream();
            IOutputChannel *savedOutput = ctx_.getOutputStream();
            IOutputChannel *savedError = ctx_.getErrorStream();
            std::unique_ptr<IInputChannel> redirectedInputChannel;
            std::unique_ptr<IOutputChannel> redirectedOutputChannel;

            std::unique_ptr<Channels::BufferedOutput> capturedOutput;

            if (currentOutputCapture_ != nullptr && !pipelineExecutionInProgress_ && !commandExpr.hasRedirectOut)
            {
                capturedOutput = std::make_unique<Channels::BufferedOutput>();
                ctx_.setOutputStream(capturedOutput.get());
            }

            if (!setContextInputChannelForRedirects(commandExpr, variables, redirectedInputChannel))
            {
                ctx_.setInputStream(savedInput);
                ctx_.setOutputStream(savedOutput);
                ctx_.setErrorStream(savedError);
                return false;
            }
            if (!setContextOutputChannelForRedirects(commandExpr, variables, redirectedOutputChannel))
            {
                ctx_.setInputStream(savedInput);
                ctx_.setOutputStream(savedOutput);
                ctx_.setErrorStream(savedError);
                return false;
            }
            if (!setContextErrorChannelForRedirects(commandExpr, variables))
            {
                ctx_.setInputStream(savedInput);
                ctx_.setOutputStream(savedOutput);
                ctx_.setErrorStream(savedError);
                return false;
            }
            // On this stage we have and know:
            // - The command keyword and arguments as strings
            // - The variables for this command execution
            // - The input/output/error channels set up according to the redirects specified in the command expression, with the input channel set to a BufferedInput containing the piped input if there is piped input, or the terminal's live input stream if there is no piped input, to allow for interactive commands that can be paused on input and resumed later.

            // We have to set up the execution context for this command based on the redirects specified in the command expression, and then execute the command via the shared pipeline executor, which will handle both simple commands and pipelines uniformly.
            // After execution, we need to capture the output and also update the runner state based on whether the command completed or is paused waiting for input or still running.
            // Problem: How can we reset the channels after execution? We want the ability to resume a command that is paused on input,
            // so we can't just reset the channels after execution because the command might still need them when it is resumed.

            // Solution: We work with a stack of StreamBundles and commands
            // Maybe we should split up the ICommand to not just have an execute() method, but also a resume() method that can be called when resuming a (paused) command.
            // Also we should rename execute() to invoke() to better reflect that it is not necessarily executing the command to completion, but just invoking it for one step, and the command itself can decide whether to return Completed, WaitingForInput, or Running.

            // What needs to be on the stack?
            // - The StreamBundle
            // - The variables reference (or a copy of the variables if they are different from the current variables, to allow for variable scope in nested calls)
            // - The command itself

            auto command = ctx_.getCommand(keyword);
            StackNode *stackTop = commandStackTop();
            const bool canResume = (stackTop != nullptr && stackTop->command == command && stackTop->invocation != nullptr &&
                                    stackTop->invocation->keyword == keyword && stackTop->invocation->arguments == arguments);

            if (!canResume)
            {
                StackNode node;
                node.command = command;
                node.invocation = std::make_unique<CommandInvocation>(keyword, arguments, CommandContext(variables), ctx_.getStreamBundle());
                commandStack_[commandStackTop_++] = std::move(node);
                stackTop = commandStackTop();
            }

            auto state = canResume ? command->resume(*stackTop->invocation) : command->invoke(*stackTop->invocation);

            if (currentOutputCapture_ != nullptr)
            {
                if (capturedOutput != nullptr)
                {
                    *currentOutputCapture_ += capturedOutput->getBuffer();
                }
                else if (stackTop != nullptr && stackTop->invocation != nullptr)
                {
                    if (stackTop->invocation->streams.output.get() != nullptr)
                    {
                        *currentOutputCapture_ += stackTop->invocation->streams.output.get()->getBuffer();
                    }
                }
            }

            ctx_.setInputStream(savedInput);
            ctx_.setOutputStream(savedOutput);
            ctx_.setErrorStream(savedError);

            // Update the command stack and runner state based on the command's execution state
            return updateCommandExecutionState(state);
        }

        bool Runner::executePipelineExpression(const PipelineExpression &pipelineExpr, ETMap<ETString, ETString> &variables)
        {
            // A pipeline expression is essentially a sequence of command expressions separated by pipes,
            // with optional input/output redirection command.

            // There have to be at least 1 command in the pipeline
            if (pipelineExpr.commands.empty())
            {
                error_ = ErrorCode::SyntaxError;
                return false;
            }

            ETString prevOutput;
            ETString *savedCapture = currentOutputCapture_;
            const bool captureRequested = (savedCapture != nullptr);
            currentOutputCapture_ = nullptr;
            pipelineExecutionInProgress_ = true;
            IInputChannel *savedInput = ctx_.getInputStream();
            IOutputChannel *savedOutput = ctx_.getOutputStream();
            IOutputChannel *savedError = ctx_.getErrorStream();

            for (size_t idx = 0; idx < pipelineExpr.commands.size(); ++idx)
            {
                const CommandExpression &commandExpr = pipelineExpr.commands[idx].data.command;

                if (idx > 0)
                {
                    ctx_.setInputStream(new Channels::BufferedInput(prevOutput));
                }

                std::unique_ptr<Channels::BufferedOutput> capturedOutput;
                // Capture output if this is not the last stage, or if the caller requested
                // output capture via currentOutputCapture_ (e.g., variable assignment).
                bool shouldCapture = !commandExpr.hasRedirectOut && (idx < pipelineExpr.commands.size() - 1 || captureRequested);
                if (shouldCapture)
                {
                    capturedOutput = std::make_unique<Channels::BufferedOutput>();
                    ctx_.setOutputStream(static_cast<IOutputChannel *>(capturedOutput.get()));
                }

                if (!executeCommandExpression(commandExpr, variables))
                {
                    pipelineExecutionInProgress_ = false;
                    currentOutputCapture_ = savedCapture;
                    ctx_.setInputStream(savedInput);
                    ctx_.setOutputStream(savedOutput);
                    ctx_.setErrorStream(savedError);
                    return false;
                }

                prevOutput = commandExpr.hasRedirectOut ? ETString() : (capturedOutput ? capturedOutput->getBuffer() : ETString());

                ctx_.setInputStream(savedInput);
                ctx_.setOutputStream(savedOutput);
                ctx_.setErrorStream(savedError);
            }

            currentOutputCapture_ = savedCapture;
            if (currentOutputCapture_ != nullptr)
            {
                *currentOutputCapture_ = prevOutput;
            }
            else if (!prevOutput.empty() && ctx_.getOutputStream() != nullptr)
            {
                ctx_.getOutputStream()->print(prevOutput);
            }

            pipelineExecutionInProgress_ = false;

            return true;
        }

        bool Runner::executeVariableAssignment(const VariableAssignmentExpression &varAssign, ETMap<ETString, ETString> &variables)
        {
            ETString value;
            switch (varAssign.valueExpression->tag)
            {
            case ExpressionType::Command:
            {
                const auto &commandExpr = varAssign.valueExpression->data.command;
                if (commandExpr.arguments.empty() && !commandExpr.hasRedirectIn && !commandExpr.hasRedirectOut &&
                    (commandExpr.keyword.type == TokenType::TRUE || commandExpr.keyword.type == TokenType::FALSE ||
                     commandExpr.keyword.text == "true" || commandExpr.keyword.text == "false"))
                {
                    value = commandExpr.keyword.text;
                    break;
                }

                ETString capturedOutput;
                ETString *previousCapture = currentOutputCapture_;
                currentOutputCapture_ = &capturedOutput;
                if (!executeCommandExpression(varAssign.valueExpression->data.command, variables))
                {
                    currentOutputCapture_ = previousCapture;
                    return false;
                }
                currentOutputCapture_ = previousCapture;
                value = capturedOutput;
                break;
            }
            case ExpressionType::FunctionCall:
            {
                ETString capturedOutput;
                ETString *previousCapture = currentOutputCapture_;
                currentOutputCapture_ = &capturedOutput;
                if (!executeFunctionCall(varAssign.valueExpression->data.functionCall, variables))
                {
                    currentOutputCapture_ = previousCapture;
                    return false;
                }
                currentOutputCapture_ = previousCapture;
                value = capturedOutput;
                break;
            }
            case ExpressionType::Nil:
                // Should not happen, raise an error
                error_ = ErrorCode::SyntaxError;
                return false;
            case ExpressionType::VariableAssignment:
            {
                // Nested variable assignment, execute it first to get the value
                if (!executeVariableAssignment(varAssign.valueExpression->data.variableAssignment, variables))
                {
                    return false;
                }
                if (!resolveTokenValue(token_t::createVariable(varAssign.valueExpression->data.variableAssignment.variable), variables, value))
                {
                    return false;
                }
                break;
            }
            case ExpressionType::Pipeline:
            {
                ETString capturedOutput;
                ETString *previousCapture = currentOutputCapture_;
                currentOutputCapture_ = &capturedOutput;
                // Pipeline expression, execute it and capture the output
                if (!executePipelineExpression(varAssign.valueExpression->data.pipeline, variables))
                {
                    currentOutputCapture_ = previousCapture;
                    return false;
                }
                currentOutputCapture_ = previousCapture;
                value = capturedOutput;
                break;
            }
            case ExpressionType::Literal:
                if (!resolveTokenValue(varAssign.valueExpression->data.literal.literalToken, variables, value))
                {
                    return false;
                }
                break;

            default:
                error_ = ErrorCode::SyntaxError;
                return false;
            }
            variables[varAssign.variable] = value;
            return true;
        }

        bool Runner::executeIfExpression(const IfExpression &ifExpr, ETMap<ETString, ETString> &variables)
        {
            for (size_t i = 0; i < ifExpr.conditions.size(); ++i)
            {
                const auto &condition = ifExpr.conditions[i];
                if (condition.tag != ExpressionType::Condition)
                {
                    error_ = ErrorCode::RuntimeError;
                    return false;
                }
                bool cond = false;

                if (!evaluateCondition(condition.data.condition, variables, cond) || error() || isPaused_())
                {
                    return false;
                }

                if (cond)
                {
                    return executeChain_(ifExpr.bodies[i], variables);
                }
            }

            return executeChain_(ifExpr.elseBody, variables);
        }

        bool Runner::executeForLoop(const ForLoopExpression &forLoop, ETMap<ETString, ETString> &variables)
        {
            for (const auto &valueToken : forLoop.values)
            {
                ETString value;
                if (!resolveTokenValue(valueToken, variables, value))
                {
                    return false;
                }
                variables[forLoop.variable] = value;
                if (!executeChain_(forLoop.body, variables))
                {
                    return false;
                }
            }
            return true;
        }

        bool Runner::executeWhileLoop(const WhileLoopExpression &whileLoop, ETMap<ETString, ETString> &variables)
        {
            while (true)
            {
                bool cond = false;
                if (whileLoop.condition->tag != ExpressionType::Condition)
                {
                    error_ = ErrorCode::RuntimeError;
                    return false;
                }
                if (!evaluateCondition(whileLoop.condition->data.condition, variables, cond) || error() || isPaused_())
                {
                    return false;
                }

                if (isPaused_())
                {
                    return false;
                }
                if (!cond)
                {
                    break;
                }
                if (!executeChain_(whileLoop.body, variables))
                {
                    return false;
                }
            }
            return true;
        }

        bool Runner::executeFunctionDefinition(const FunctionDefinitionExpression &funcDef, ETMap<ETString, ETString> &variables)
        {
            (void)variables;
            if (ctx_.existsFunction(funcDef.functionName))
            {
                error_ = ErrorCode::SyntaxError;
                return false;
            }

            ctx_.registerFunction(funcDef.functionName, funcDef);
            return true;
        }

        bool Runner::executeFunctionCall(const FunctionCallExpression &funcCall, ETMap<ETString, ETString> &variables)
        {
            const auto *funcDef = ctx_.getFunction(funcCall.functionName);
            if (funcDef == nullptr)
            {
                error_ = ErrorCode::SyntaxError;
                return false;
            }
            const auto &funcDefRef = *funcDef;
            if (funcCall.arguments.size() != funcDefRef.parameters.size())
            {
                error_ = ErrorCode::SyntaxError;
                return false;
            }

            ETMap<ETString, ETString> functionScopeVariables = variables;
            for (size_t i = 0; i < funcDefRef.parameters.size(); ++i)
            {
                ETString argumentValue;
                if (!resolveTokenValue(funcCall.arguments[i], variables, argumentValue))
                {
                    return false;
                }
                functionScopeVariables[funcDefRef.parameters[i]] = argumentValue;
            }

            return executeChain_(funcDefRef.body, functionScopeVariables);
        }

        bool Runner::evaluateCondition(const ConditionExpression &condition, ETMap<ETString, ETString> &variables, bool &result)
        {
            // Instead of relying on an external Evaluator, we can evaluate the condition directly here in the Runner,
            // since we have access to the variables and can execute command blocks as needed.
            // This also allows us to properly handle pausing if the condition involves a command block that is waiting for input.

            // We can implement a simple recursive evaluation of the condition expression tree here, evaluating command blocks as needed and returning the final boolean result.
            // For simplicity, let's assume the condition expression is in Conjunctive Normal Form (CNF) where we have ANDs of ORs of simple conditions, and the leaf nodes can be either a simple condition (like "VAR == value") or a command block whose output we interpret as boolean.

            if (condition.conditionNodes.size() != condition.connectingOperators.size() + 1)
            {
                error_ = ErrorCode::SyntaxError;
                return false;
            }

            // The order of operator execution are
            // 1. Node
            // 2. NOT
            // 3. AND
            // 4. OR

            // We can evaluate the condition using a stack to handle operator precedence
            ETVector<bool> valueStack;
            for (size_t i = 0; i < condition.conditionNodes.size(); ++i)
            {
                const auto &node = condition.conditionNodes[i];
                bool nodeValue = false;
                switch (node.first.tag)
                {
                case ExpressionType::Literal:
                {
                    const auto &literalToken = node.first.data.literal.literalToken;
                    if (literalToken.type == TokenType::WORD && ctx_.existsCommand(literalToken.text))
                    {
                        Expression commandExpression = Expression::createCommand(literalToken, token_list_t());
                        if (!executeCommandExpression(commandExpression.data.command, variables) || error() || isPaused_())
                        {
                            return false;
                        }
                        nodeValue = (lastCommandExitCode_ == 0);
                    }
                    else
                    {
                        bool tokenValue = evalTokenToBool(literalToken, variables);
                        nodeValue = tokenValue;
                        if (error())
                        {
                            return false;
                        }
                    }
                    if (node.second)
                    {
                        nodeValue = !nodeValue;
                    }
                }
                break;
                case ExpressionType::Condition:
                    if (!evaluateCondition(node.first.data.condition, variables, nodeValue) || error() || isPaused_())
                    {
                        return false;
                    }
                    if (isPaused_())
                    {
                        return false;
                    }
                    if (node.second)
                    {
                        nodeValue = !nodeValue;
                    }
                    break;
                case ExpressionType::Pipeline:
                {
                    ETString capturedOutput;
                    ETString *previousCapture = currentOutputCapture_;
                    currentOutputCapture_ = &capturedOutput;
                    if (!executePipelineExpression(node.first.data.pipeline, variables))
                    {
                        currentOutputCapture_ = previousCapture;
                        return false;
                    }
                    currentOutputCapture_ = previousCapture;
                    nodeValue = evalStringToBool(capturedOutput);
                    if (node.second)
                    {
                        nodeValue = !nodeValue;
                    }
                    break;
                }
                case ExpressionType::Comparison:
                    if (!evaluateComparison(node.first.data.comparison, variables, nodeValue) || error() || isPaused_())
                    {
                        return false;
                    }
                    if (node.second)
                    {
                        nodeValue = !nodeValue;
                    }
                    break;
                default:
                    error_ = ErrorCode::RuntimeError;
                    return false;
                }

                valueStack.push_back(nodeValue);
            }

            for (size_t i = 0; i < condition.connectingOperators.size(); ++i)
            {
                const auto &op = condition.connectingOperators[i];
                if (op == TokenType::AND_AND)
                {
                    if (valueStack.size() < 2)
                    {
                        error_ = ErrorCode::RuntimeError;
                        return false;
                    }
                    bool rhs = valueStack.back();
                    valueStack.pop_back();
                    bool lhs = valueStack.back();
                    valueStack.pop_back();
                    valueStack.push_back(lhs && rhs);
                }
                else if (op == TokenType::OR_OR)
                {
                    if (valueStack.size() < 2)
                    {
                        error_ = ErrorCode::RuntimeError;
                        return false;
                    }
                    bool rhs = valueStack.back();
                    valueStack.pop_back();
                    bool lhs = valueStack.back();
                    valueStack.pop_back();
                    valueStack.push_back(lhs || rhs);
                }
                else
                {
                    error_ = ErrorCode::RuntimeError;
                    return false;
                }
            }

            result = valueStack.empty() ? false : valueStack.back();
            return !valueStack.empty();
        }

        bool Runner::evaluateComparison(const ComparisonExpression &comparison, ETMap<ETString, ETString> &variables, bool &result)
        {
            ETString leftValue;
            ETString rightValue;

            if (!resolveExpressionToString(comparison.leftOperand.get(), variables, leftValue))
            {
                return false;
            }

            if (!resolveExpressionToString(comparison.rightOperand.get(), variables, rightValue))
            {
                return false;
            }

            // Perform comparison according to operator
            switch (comparison.operatorType)
            {
            case TokenType::EQUALS_EQUALS:
                result = (leftValue == rightValue);
                return true;
            case TokenType::NOT_EQUALS:
                result = (leftValue != rightValue);
                return true;
            default:
                error_ = ErrorCode::SyntaxError;
                return false;
            }
        }

        bool Runner::resolveExpressionToString(const Expression *expr, ETMap<ETString, ETString> &variables, ETString &out)
        {
            if (expr == nullptr)
            {
                error_ = ErrorCode::RuntimeError;
                return false;
            }
            switch (expr->tag)
            {
            case ExpressionType::Literal:
                if (!resolveTokenValue(expr->data.literal.literalToken, variables, out))
                {
                    return false;
                }
                return true;
            case ExpressionType::Pipeline:
            {
                ETString *previousCapture = currentOutputCapture_;
                currentOutputCapture_ = &out;
                if (!executePipelineExpression(expr->data.pipeline, variables))
                {
                    currentOutputCapture_ = previousCapture;
                    return false;
                }
                currentOutputCapture_ = previousCapture;
                return true;
            }
            case ExpressionType::FunctionCall:
            {
                ETString *previousCapture = currentOutputCapture_;
                currentOutputCapture_ = &out;
                if (!executeFunctionCall(expr->data.functionCall, variables))
                {
                    currentOutputCapture_ = previousCapture;
                    return false;
                }
                currentOutputCapture_ = previousCapture;
                return true;
            }
            default:
                error_ = ErrorCode::RuntimeError;
                return false;
            }
        }
    } // namespace Scripting
} // namespace EmbeddedTerminal
