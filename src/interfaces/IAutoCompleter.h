#ifndef IAUTOCOPMLETER_H
#define IAUTOCOPMLETER_H

#include "ETTypes.h"

namespace EmbeddedTerminal
{
    /// @brief Interface for providing auto completion suggestions
    /// Commands can optionally implement this interface to provide context-aware
    /// auto completion when the user presses TAB during input.
    class IAutoCompleter
    {
    public:
        virtual ~IAutoCompleter() = default;

        /// @brief Get auto completion suggestions for a partial input string
        /// @param partial The incomplete text being completed (e.g., "tm" when completing "/tm")
        /// @return Vector of completion suggestions that start with the partial string
        virtual ETVector<ETString> getSuggestions(const ETString &partial) = 0;
    };
} // namespace EmbeddedTerminal
#endif // IAUTOCOPMLETER_H
