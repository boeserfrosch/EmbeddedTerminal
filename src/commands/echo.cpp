#include "commands/echo.h"

using namespace EmbeddedTerminal::cmd;

ETString echo::trigger(const ETString &keyword, const ETString &additional)
{
    (void)keyword;
    return additional + "\n";
}

ETString echo::usage(const ETString &keyword)
{
    return keyword + " <text> - Print the specified text\n";
}
