#ifndef IAUTOCOPMLETER_H
#define IAUTOCOPMLETER_H

#include "ETTypes.h"

namespace EmbeddedTerminal
{
    /**
     * @brief Interface for auto-completion functionality in terminal commands.
     *
     * Implementations provide suggestions for command completion based on user input.
     */
    class IAutoCompleter
    {
    public:
        virtual ~IAutoCompleter() = default;

        /**
         * @brief Suggests possible completions for the given input.
         * @param partial The current user input string.
         * @return A sorted list of possible completions.
         */
        virtual ETVector<ETString> getSuggestions(const ETString &partial) = 0;
    };
} // namespace EmbeddedTerminal
#endif // IAUTOCOPMLETER_H
