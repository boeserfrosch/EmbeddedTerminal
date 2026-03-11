#ifndef TERMINAL_PARSER_H
#define TERMINAL_PARSER_H

#include "TerminalTokenizer.h"
#include "TerminalAst.h"

namespace EmbeddedTerminal
{
    class TerminalParser
    {
    public:
        bool parseTokens(const ETVector<token_t> &tokens, ParsedAst &ast) const;
        bool extractFunctionDefs(ETVector<token_t> &tokens, ETVector<ParsedFunctionDef> &outDefs) const;

    private:
        bool parseCommandTokens_(const ETVector<token_t> &tokens, size_t start, size_t end, ParsedCommand &out) const;
        bool parseChainTokens_(const ETVector<token_t> &tokens, size_t start, size_t end, ParsedChain &out) const;
        bool parseForLoopTokens_(const ETVector<token_t> &tokens, size_t start, size_t end, ParsedForLoop &out) const;
        bool parseWhileLoopTokens_(const ETVector<token_t> &tokens, size_t start, size_t end, ParsedWhileLoop &out) const;
        bool parseIfBlockTokens_(const ETVector<token_t> &tokens, size_t start, size_t end, ParsedIfBlock &out) const;

        bool tokenToCommandText_(const token_t &token, ETString &out) const;
        void appendWithSpace_(ETString &target, const ETString &text) const;
    };
}

#endif // TERMINAL_PARSER_H
