#include "commands/pwd.h"

using namespace EmbeddedTerminal::cmd;

ETString pwd::trigger(const ETString &keyword, const ETString &additional)
{
    // pwd takes no parameters, just return current directory
    return _dir.pwd() + "\n";
}

ETString pwd::usage(const ETString &keyword)
{
    return keyword + " - Print the current working directory\n";
}
