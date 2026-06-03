#pragma once
#include "lang/LangAPI.h"
#include "interfaces/IExecutionContext.h"
#include "Parser.h"

namespace EmbeddedTerminal
{
    using namespace Lang;

    namespace Scripting
    {

        class Runner
        {

        public:
            enum State
            {
                Error,
                Running,
                Completed
            };
            enum ErrorCode
            {
                None,
                SyntaxError,
                EmptyCondition,
                UndefinedVariable,
                UnterminatedStringLiteral,
                RuntimeError,
                CommandNotFound,
                StackOverflow
            };

        private:
            struct StackNode
            {
                // We need to store the context for each command in the call stack, so that when we resume a paused command, we can restore the correct context (including variables and I/O channels) for that command.
                ICommand *command;
                std::unique_ptr<CommandInvocation> invocation;
                CommandExecutionState state = CommandExecutionState::Completed;

                StackNode(ICommand *cmd, std::unique_ptr<CommandInvocation> inv) : command(cmd), invocation(std::move(inv))
                {
                }

                StackNode() : command(nullptr), invocation(nullptr)
                {
                }

                // Disable copying - StackNode owns unique command invocation resources
                StackNode(const StackNode &other) = delete;
                StackNode &operator=(const StackNode &other) = delete;
                StackNode(StackNode &&other) = default;
                StackNode &operator=(StackNode &&other) = default;

                ~StackNode()
                {
                    // Ensure owned invocation resources are released. Do NOT call
                    // command->onInterrupt() here — interrupt semantics should be
                    // driven explicitly by Runner::interrupt() to avoid unexpectedly
                    // interrupting background or running commands during normal
                    // stack cleanup.
                    if (invocation)
                    {
                        invocation.reset();
                    }
                    command = nullptr;
                    invocation = nullptr;
                }
            };

            // Instead of working with a dynamic sized call stack, we use a fixed size array to store the call stack for simplicity and to avoid dynamic memory allocation during command execution, which can be important in embedded environments. We can define a maximum call stack depth (e.g. 16) and use an index to keep track of the current top of the stack.
            static const size_t MaxCommandStackDepth = 16;
            StackNode commandStack_[MaxCommandStackDepth];
            size_t commandStackTop_ = 0;

            IExecutionContext &ctx_;
            ErrorCode error_ = ErrorCode::None;
            const ExpressionChain *currentAst_ = nullptr;
            size_t currentIndex_ = 0;
            ETMap<ETString, ETString> *currentVariables_ = nullptr;
            struct CallFrame
            {
                const ExpressionChain *ast = nullptr;
                size_t index = 0;
                ETMap<ETString, ETString> *previousVariables = nullptr;
                ETMap<ETString, ETString> ownedVariables;
                bool ownsVariables = false;
            };
            ETVector<CallFrame> callStack_;
            ETString *currentOutputCapture_ = nullptr;
            bool pipelineExecutionInProgress_ = false;
            int32_t lastCommandExitCode_ = 0;

        public:
            explicit Runner(IExecutionContext &ctx) : ctx_(ctx) {};
            State execute(const ExpressionChain &ast, ETMap<ETString, ETString> &variables);

            void reset();
            bool isInitialized() const
            {
                return (currentAst_ != nullptr);
            }
            void interrupt();
            bool error() const
            {
                return error_ != ErrorCode::None;
            }
            ErrorCode getLastError() const
            {
                return error_;
            }
            CommandExecutionState getLastPauseState() const
            {
                const StackNode *stackTop = commandStackTop_ == 0 ? nullptr : &commandStack_[commandStackTop_ - 1];
                return stackTop == nullptr ? CommandExecutionState::Completed : stackTop->state;
            }

        protected:
            bool isPaused_() const
            {
                return commandStackTop_ != 0;
            }
            bool executeChain_(const ExpressionChain &ast, ETMap<ETString, ETString> &variables);
            bool executeExpression_(const Expression &expression, ETMap<ETString, ETString> &variables);
            bool executeCommandExpression(const CommandExpression &commandExpr, ETMap<ETString, ETString> &variables);
            bool executePipelineExpression(const PipelineExpression &pipelineExpr, ETMap<ETString, ETString> &variables);

            bool executeVariableAssignment(const VariableAssignmentExpression &varAssign, ETMap<ETString, ETString> &variables);
            bool executeIfExpression(const IfExpression &ifExpr, ETMap<ETString, ETString> &variables);
            bool executeForLoop(const ForLoopExpression &forLoop, ETMap<ETString, ETString> &variables);
            bool executeWhileLoop(const WhileLoopExpression &whileLoop, ETMap<ETString, ETString> &variables);
            bool executeFunctionDefinition(const FunctionDefinitionExpression &funcDef, ETMap<ETString, ETString> &variables);
            bool executeFunctionCall(const FunctionCallExpression &funcCall, ETMap<ETString, ETString> &variables);

            bool evaluateCondition(const ConditionExpression &condition, ETMap<ETString, ETString> &variables, bool &result);
            bool evaluateComparison(const ComparisonExpression &comparison, ETMap<ETString, ETString> &variables, bool &result);

            // Resolve an Expression (Literal, Pipeline, FunctionCall) to a string value.
            // Uses currentOutputCapture_ when executing pipelines or function calls so callers
            // can capture output produced by those expressions.
            bool resolveExpressionToString(const Expression *expr, ETMap<ETString, ETString> &variables, ETString &out);

        private:
            void resetExecutionState_();

            bool setContextInputChannelForRedirects(const CommandExpression &commandExpr, ETMap<ETString, ETString> &variables, std::unique_ptr<IInputChannel> &ownedInputChannel);
            bool setContextOutputChannelForRedirects(const CommandExpression &commandExpr, ETMap<ETString, ETString> &variables, std::unique_ptr<IOutputChannel> &ownedOutputChannel);
            bool setContextErrorChannelForRedirects(const CommandExpression &commandExpr, ETMap<ETString, ETString> &variables);

            bool resolveKeywordAndArguments(const CommandExpression &commandExpr, ETMap<ETString, ETString> &variables, ETString &keyword, ETVector<ETString> &arguments);

            bool updateCommandExecutionState(CommandResult result);
            StackNode *commandStackTop();

            /// @brief Resolves a token to a string value. This is used for resolving literal expressions, and also for resolving the keyword and arguments of command expressions, which can contain variable references and need to be resolved before we can execute the command.
            /// @param token The token to resolve. This can be a WORD, a STRING_LITERAL, a VARIABLE token, etc.
            /// @param variables The current variable map, used for resolving VARIABLE tokens. This should be passed in from the caller so that we can correctly resolve variables in the context of the current execution (e.g. including variables defined in the current function or block scope).
            /// @param value The resolved string value.
            /// @return True if the token was resolved successfully, false otherwise.
            bool resolveTokenValue(const token_t &token, ETMap<ETString, ETString> &variables, ETString &value);
            bool evalTokenToBool(const token_t &token, ETMap<ETString, ETString> &variables);
        };
    }

} // namespace name
