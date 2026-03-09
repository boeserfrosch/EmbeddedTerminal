#ifndef SHELL_VARIABLE_STORE_H
#define SHELL_VARIABLE_STORE_H

#include <stddef.h>
#include <stdint.h>

#ifndef SHELL_VAR_MAX
#define SHELL_VAR_MAX 64
#endif

#ifndef SHELL_VAR_NAME_MAX
#define SHELL_VAR_NAME_MAX 31
#endif

#ifndef SHELL_VAR_VALUE_MAX
#define SHELL_VAR_VALUE_MAX 127
#endif

namespace EmbeddedTerminal
{
    enum ShellError
    {
        SHELL_OK = 0,
        SHELL_ERR_INVALID_ARG,
        SHELL_ERR_TABLE_FULL,
        SHELL_ERR_NOT_FOUND,
        SHELL_ERR_PARSE,
        SHELL_ERR_DIV_ZERO,
        SHELL_ERR_TRUNCATED
    };

    struct shell_var_entry_t
    {
        bool used;
        char name[SHELL_VAR_NAME_MAX + 1];
        char value[SHELL_VAR_VALUE_MAX + 1];
    };

    struct shell_var_store_t
    {
        shell_var_entry_t entries[SHELL_VAR_MAX];
        int lastStatus;
        int argc;
        char positional[10][SHELL_VAR_VALUE_MAX + 1];
    };

    void shellVarStoreInit(shell_var_store_t &store);
    ShellError shellVarSet(shell_var_store_t &store, const char *name, const char *value);
    ShellError shellVarUnset(shell_var_store_t &store, const char *name);
    bool shellVarGet(const shell_var_store_t &store, const char *name, const char *&valueOut);

    void shellVarSetLastStatus(shell_var_store_t &store, int status);
    void shellVarSetArgCount(shell_var_store_t &store, int argc);
    ShellError shellVarSetPositional(shell_var_store_t &store, int index, const char *value);

    ShellError shellExpand(const shell_var_store_t &store, const char *input, char *dst, size_t dstLen);
} // namespace EmbeddedTerminal

#endif // SHELL_VARIABLE_STORE_H
