#ifndef ET_UTILS_CONVERSION_H
#define ET_UTILS_CONVERSION_H

#include "ETTypes.h"
#include <string>
#include <limits>

namespace EmbeddedTerminal
{
    namespace utils
    {

        inline bool to_int(const ETString &s, int &out)
        {
            if (s.empty())
                return false;
            try
            {
                size_t processed = 0;
                long long v = std::stoll(s.c_str(), &processed, 10);
                if (processed == 0)
                    return false;
                if (v < std::numeric_limits<int>::min() || v > std::numeric_limits<int>::max())
                    return false;
                out = static_cast<int>(v);
                return true;
            }
            catch (...)
            {
                return false;
            }
        }

        inline bool to_size_t(const ETString &s, size_t &out)
        {
            if (s.empty())
                return false;
            try
            {
                size_t processed = 0;
                unsigned long long v = std::stoull(s.c_str(), &processed, 10);
                if (processed == 0)
                    return false;
                if (v > static_cast<unsigned long long>(std::numeric_limits<size_t>::max()))
                    return false;
                out = static_cast<size_t>(v);
                return true;
            }
            catch (...)
            {
                return false;
            }
        }

        inline ETString from_int(int v)
        {
            return toETString(static_cast<int32_t>(v));
        }

        inline ETString from_size_t(size_t v)
        {
            return toETString(static_cast<size_t>(v));
        }

    } // namespace utils
} // namespace EmbeddedTerminal

#endif // ET_UTILS_CONVERSION_H
