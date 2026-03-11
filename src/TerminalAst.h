#ifndef TERMINAL_AST_H
#define TERMINAL_AST_H

#include "ETTypes.h"

namespace EmbeddedTerminal
{
    enum class ChainCondition
    {
        Always,
        OnSuccess,
        OnFailure
    };

    struct ParsedCommand
    {
        ETVector<ETString> keywords;
        ETVector<ETString> arguments;
        ETString redirectOutPath;
        bool appendRedirect = false;
        ETString redirectInPath;
    };

    struct ParsedChainSegment
    {
        ParsedCommand command;
        ChainCondition condition = ChainCondition::Always;
    };

    struct ParsedChain
    {
        ETVector<ParsedChainSegment> segments;
    };

    struct ParsedForLoop
    {
        ETString variable;
        ETVector<ETString> values;
        ParsedChain body;
    };

    struct ParsedWhileLoop
    {
        bool hasLiteralCondition = false;
        bool literalCondition = false;
        ParsedChain condition;
        ParsedChain body;
    };

    struct ParsedIfBlock
    {
        ParsedChain condition;
        ParsedChain thenBody;
        ETVector<ParsedChain> elifConditions;
        ETVector<ParsedChain> elifBodies;
        bool hasElse = false;
        ParsedChain elseBody;
    };

    struct ParsedFunctionDef
    {
        ETString name;
        ParsedChain body;
    };

    struct ParsedAst
    {
        bool isForLoop = false;
        bool isWhileLoop = false;
        bool isIfBlock = false;
        ParsedChain chain;
        ParsedForLoop forLoop;
        ParsedWhileLoop whileLoop;
        ParsedIfBlock ifBlock;
    };
}

#endif // TERMINAL_AST_H
