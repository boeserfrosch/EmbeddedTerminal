#include "ShellVariableStore.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

namespace
{
    bool isNameStart(char c)
    {
        return (c == '_') || ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z'));
    }

    bool isNameChar(char c)
    {
        return isNameStart(c) || ((c >= '0') && (c <= '9'));
    }

    bool safeCopy(char *dst, size_t dstSize, const char *src)
    {
        if ((dst == nullptr) || (src == nullptr) || (dstSize == 0))
        {
            return false;
        }

        size_t srcLen = strlen(src);
        size_t copyLen = srcLen < (dstSize - 1) ? srcLen : (dstSize - 1);
        if (copyLen > 0)
        {
            memcpy(dst, src, copyLen);
        }
        dst[copyLen] = '\0';
        return srcLen < dstSize;
    }

    bool appendChar(char *dst, size_t dstLen, size_t &outPos, char c)
    {
        if (dstLen == 0)
        {
            return false;
        }

        if (outPos + 1 >= dstLen)
        {
            dst[dstLen - 1] = '\0';
            return false;
        }

        dst[outPos++] = c;
        dst[outPos] = '\0';
        return true;
    }

    bool appendString(char *dst, size_t dstLen, size_t &outPos, const char *text)
    {
        if (text == nullptr)
        {
            return true;
        }

        while (*text != '\0')
        {
            if (!appendChar(dst, dstLen, outPos, *text))
            {
                return false;
            }
            ++text;
        }

        return true;
    }

    bool intToString(long long value, char *buffer, size_t bufferLen)
    {
        if ((buffer == nullptr) || (bufferLen == 0))
        {
            return false;
        }

        int written = snprintf(buffer, bufferLen, "%lld", value);
        return (written >= 0) && ((size_t)written < bufferLen);
    }

    struct ExprParser
    {
        const char *text;
        size_t length;
        size_t pos;
        bool ok;
        bool divZero;
    };

    void skipWs(ExprParser &p)
    {
        while ((p.pos < p.length) && isspace((unsigned char)p.text[p.pos]))
        {
            ++p.pos;
        }
    }

    long long parseExpression(ExprParser &p);

    long long parseFactor(ExprParser &p)
    {
        skipWs(p);
        if (!p.ok)
        {
            return 0;
        }

        if (p.pos >= p.length)
        {
            p.ok = false;
            return 0;
        }

        if (p.text[p.pos] == '(')
        {
            ++p.pos;
            long long value = parseExpression(p);
            skipWs(p);
            if ((p.pos >= p.length) || (p.text[p.pos] != ')'))
            {
                p.ok = false;
                return 0;
            }
            ++p.pos;
            return value;
        }

        if (p.text[p.pos] == '+')
        {
            ++p.pos;
            return parseFactor(p);
        }

        if (p.text[p.pos] == '-')
        {
            ++p.pos;
            return -parseFactor(p);
        }

        bool hasDigits = false;
        long long value = 0;
        while ((p.pos < p.length) && isdigit((unsigned char)p.text[p.pos]))
        {
            hasDigits = true;
            value = (value * 10) + (p.text[p.pos] - '0');
            ++p.pos;
        }

        if (!hasDigits)
        {
            p.ok = false;
            return 0;
        }

        return value;
    }

    long long parseTerm(ExprParser &p)
    {
        long long lhs = parseFactor(p);

        while (p.ok)
        {
            skipWs(p);
            if (p.pos >= p.length)
            {
                break;
            }

            char op = p.text[p.pos];
            if ((op != '*') && (op != '/') && (op != '%'))
            {
                break;
            }

            ++p.pos;
            long long rhs = parseFactor(p);
            if (!p.ok)
            {
                return 0;
            }

            if (op == '*')
            {
                lhs *= rhs;
            }
            else if (op == '/')
            {
                if (rhs == 0)
                {
                    p.ok = false;
                    p.divZero = true;
                    return 0;
                }
                lhs /= rhs;
            }
            else
            {
                if (rhs == 0)
                {
                    p.ok = false;
                    p.divZero = true;
                    return 0;
                }
                lhs %= rhs;
            }
        }

        return lhs;
    }

    long long parseExpression(ExprParser &p)
    {
        long long lhs = parseTerm(p);

        while (p.ok)
        {
            skipWs(p);
            if (p.pos >= p.length)
            {
                break;
            }

            char op = p.text[p.pos];
            if ((op != '+') && (op != '-'))
            {
                break;
            }

            ++p.pos;
            long long rhs = parseTerm(p);
            if (!p.ok)
            {
                return 0;
            }

            if (op == '+')
            {
                lhs += rhs;
            }
            else
            {
                lhs -= rhs;
            }
        }

        return lhs;
    }

    bool evalIntegerExpr(const char *expr, size_t exprLen, long long &outValue, bool &divZero)
    {
        ExprParser parser = {expr, exprLen, 0, true, false};
        long long value = parseExpression(parser);
        skipWs(parser);

        if (!parser.ok || (parser.pos != parser.length))
        {
            divZero = parser.divZero;
            return false;
        }

        outValue = value;
        divZero = false;
        return true;
    }
} // namespace

void EmbeddedTerminal::shellVarStoreInit(shell_var_store_t &store)
{
    store.lastStatus = 0;
    store.argc = 0;

    for (size_t i = 0; i < SHELL_VAR_MAX; ++i)
    {
        store.entries[i].used = false;
        store.entries[i].name[0] = '\0';
        store.entries[i].value[0] = '\0';
    }

    for (size_t i = 0; i < 10; ++i)
    {
        store.positional[i][0] = '\0';
    }
}

EmbeddedTerminal::ShellError EmbeddedTerminal::shellVarSet(shell_var_store_t &store, const char *name, const char *value)
{
    if ((name == nullptr) || (value == nullptr) || !isNameStart(name[0]))
    {
        return SHELL_ERR_INVALID_ARG;
    }

    for (size_t i = 1; name[i] != '\0'; ++i)
    {
        if (!isNameChar(name[i]))
        {
            return SHELL_ERR_INVALID_ARG;
        }
    }

    size_t freeIndex = SHELL_VAR_MAX;
    for (size_t i = 0; i < SHELL_VAR_MAX; ++i)
    {
        if (store.entries[i].used)
        {
            if (strcmp(store.entries[i].name, name) == 0)
            {
                if (!safeCopy(store.entries[i].value, sizeof(store.entries[i].value), value))
                {
                    return SHELL_ERR_TRUNCATED;
                }
                return SHELL_OK;
            }
        }
        else if (freeIndex == SHELL_VAR_MAX)
        {
            freeIndex = i;
        }
    }

    if (freeIndex == SHELL_VAR_MAX)
    {
        return SHELL_ERR_TABLE_FULL;
    }

    store.entries[freeIndex].used = true;
    bool nameOk = safeCopy(store.entries[freeIndex].name, sizeof(store.entries[freeIndex].name), name);
    bool valueOk = safeCopy(store.entries[freeIndex].value, sizeof(store.entries[freeIndex].value), value);

    if (!nameOk || !valueOk)
    {
        return SHELL_ERR_TRUNCATED;
    }

    return SHELL_OK;
}

EmbeddedTerminal::ShellError EmbeddedTerminal::shellVarUnset(shell_var_store_t &store, const char *name)
{
    if (name == nullptr)
    {
        return SHELL_ERR_INVALID_ARG;
    }

    for (size_t i = 0; i < SHELL_VAR_MAX; ++i)
    {
        if (store.entries[i].used && (strcmp(store.entries[i].name, name) == 0))
        {
            store.entries[i].used = false;
            store.entries[i].name[0] = '\0';
            store.entries[i].value[0] = '\0';
            return SHELL_OK;
        }
    }

    return SHELL_ERR_NOT_FOUND;
}

bool EmbeddedTerminal::shellVarGet(const shell_var_store_t &store, const char *name, const char *&valueOut)
{
    valueOut = nullptr;
    if (name == nullptr)
    {
        return false;
    }

    for (size_t i = 0; i < SHELL_VAR_MAX; ++i)
    {
        if (store.entries[i].used && (strcmp(store.entries[i].name, name) == 0))
        {
            valueOut = store.entries[i].value;
            return true;
        }
    }

    return false;
}

void EmbeddedTerminal::shellVarSetLastStatus(shell_var_store_t &store, int status)
{
    store.lastStatus = status;
}

void EmbeddedTerminal::shellVarSetArgCount(shell_var_store_t &store, int argc)
{
    store.argc = argc;
}

EmbeddedTerminal::ShellError EmbeddedTerminal::shellVarSetPositional(shell_var_store_t &store, int index, const char *value)
{
    if ((index < 0) || (index > 9) || (value == nullptr))
    {
        return SHELL_ERR_INVALID_ARG;
    }

    if (!safeCopy(store.positional[index], sizeof(store.positional[index]), value))
    {
        return SHELL_ERR_TRUNCATED;
    }

    return SHELL_OK;
}

EmbeddedTerminal::ShellError EmbeddedTerminal::shellExpand(const shell_var_store_t &store, const char *input, char *dst, size_t dstLen)
{
    if ((input == nullptr) || (dst == nullptr) || (dstLen == 0))
    {
        return SHELL_ERR_INVALID_ARG;
    }

    size_t outPos = 0;
    dst[0] = '\0';
    bool wasTruncated = false;

    size_t pos = 0;
    while (input[pos] != '\0')
    {
        if (input[pos] != '$')
        {
            if (!appendChar(dst, dstLen, outPos, input[pos]))
            {
                wasTruncated = true;
            }
            ++pos;
            continue;
        }

        if (input[pos + 1] == '\0')
        {
            if (!appendChar(dst, dstLen, outPos, '$'))
            {
                wasTruncated = true;
            }
            ++pos;
            continue;
        }

        if (input[pos + 1] == '?')
        {
            char number[24];
            if (!intToString((long long)store.lastStatus, number, sizeof(number)) || !appendString(dst, dstLen, outPos, number))
            {
                wasTruncated = true;
            }
            pos += 2;
            continue;
        }

        if (input[pos + 1] == '#')
        {
            char number[24];
            if (!intToString((long long)store.argc, number, sizeof(number)) || !appendString(dst, dstLen, outPos, number))
            {
                wasTruncated = true;
            }
            pos += 2;
            continue;
        }

        if ((input[pos + 1] >= '0') && (input[pos + 1] <= '9'))
        {
            int index = input[pos + 1] - '0';
            if (!appendString(dst, dstLen, outPos, store.positional[index]))
            {
                wasTruncated = true;
            }
            pos += 2;
            continue;
        }

        if ((input[pos + 1] == '(') && (input[pos + 2] == '('))
        {
            size_t exprStart = pos + 3;
            size_t scan = exprStart;
            int parenDepth = 0;
            while (input[scan] != '\0')
            {
                if ((parenDepth == 0) && (input[scan] == ')') && (input[scan + 1] == ')'))
                {
                    break;
                }

                if (input[scan] == '(')
                {
                    ++parenDepth;
                }
                else if (input[scan] == ')')
                {
                    if (parenDepth > 0)
                    {
                        --parenDepth;
                    }
                    else
                    {
                        return SHELL_ERR_PARSE;
                    }
                }
                ++scan;
            }

            if (input[scan] == '\0')
            {
                return SHELL_ERR_PARSE;
            }

            long long exprValue = 0;
            bool divZero = false;
            if (!evalIntegerExpr(&input[exprStart], scan - exprStart, exprValue, divZero))
            {
                return divZero ? SHELL_ERR_DIV_ZERO : SHELL_ERR_PARSE;
            }

            char number[32];
            if (!intToString(exprValue, number, sizeof(number)) || !appendString(dst, dstLen, outPos, number))
            {
                wasTruncated = true;
            }

            pos = scan + 2;
            continue;
        }

        if (input[pos + 1] == '{')
        {
            size_t nameStart = pos + 2;
            if (!isNameStart(input[nameStart]))
            {
                return SHELL_ERR_PARSE;
            }

            size_t nameEnd = nameStart + 1;
            while (isNameChar(input[nameEnd]))
            {
                ++nameEnd;
            }

            char name[SHELL_VAR_NAME_MAX + 1];
            size_t nameLen = nameEnd - nameStart;
            if (nameLen > SHELL_VAR_NAME_MAX)
            {
                nameLen = SHELL_VAR_NAME_MAX;
            }
            memcpy(name, &input[nameStart], nameLen);
            name[nameLen] = '\0';

            const char *resolved = nullptr;
            bool found = shellVarGet(store, name, resolved);
            bool hasValue = found && (resolved != nullptr) && (resolved[0] != '\0');

            if ((input[nameEnd] == ':') && (input[nameEnd + 1] == '-'))
            {
                size_t defaultStart = nameEnd + 2;
                size_t scan = defaultStart;
                while ((input[scan] != '\0') && (input[scan] != '}'))
                {
                    ++scan;
                }

                if (input[scan] != '}')
                {
                    return SHELL_ERR_PARSE;
                }

                if (hasValue)
                {
                    if (!appendString(dst, dstLen, outPos, resolved))
                    {
                        wasTruncated = true;
                    }
                }
                else
                {
                    while (defaultStart < scan)
                    {
                        if (!appendChar(dst, dstLen, outPos, input[defaultStart]))
                        {
                            wasTruncated = true;
                        }
                        ++defaultStart;
                    }
                }

                pos = scan + 1;
                continue;
            }

            if (input[nameEnd] != '}')
            {
                return SHELL_ERR_PARSE;
            }

            if (found && (resolved != nullptr))
            {
                if (!appendString(dst, dstLen, outPos, resolved))
                {
                    wasTruncated = true;
                }
            }

            pos = nameEnd + 1;
            continue;
        }

        if (isNameStart(input[pos + 1]))
        {
            size_t nameStart = pos + 1;
            size_t nameEnd = nameStart + 1;
            while (isNameChar(input[nameEnd]))
            {
                ++nameEnd;
            }

            char name[SHELL_VAR_NAME_MAX + 1];
            size_t nameLen = nameEnd - nameStart;
            if (nameLen > SHELL_VAR_NAME_MAX)
            {
                nameLen = SHELL_VAR_NAME_MAX;
            }
            memcpy(name, &input[nameStart], nameLen);
            name[nameLen] = '\0';

            const char *resolved = nullptr;
            if (shellVarGet(store, name, resolved) && (resolved != nullptr))
            {
                if (!appendString(dst, dstLen, outPos, resolved))
                {
                    wasTruncated = true;
                }
            }

            pos = nameEnd;
            continue;
        }

        if (!appendChar(dst, dstLen, outPos, '$'))
        {
            wasTruncated = true;
        }
        ++pos;
    }

    return wasTruncated ? SHELL_ERR_TRUNCATED : SHELL_OK;
}
