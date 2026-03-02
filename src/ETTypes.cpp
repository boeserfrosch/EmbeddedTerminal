#include "ETTypes.h"
#include <cctype>
#include <cstdio>
#include <stdexcept>
#include <memory>
#include <algorithm>

// ETString implementation
ETString::ETString() : data("")

{
}

ETString::ETString(const char *s) : data(s) {}
ETString::ETString(const unsigned char *s) : data((const char *)s) {}

ETString::ETString(const ETString &other) : data(other.data) {}
ETString &ETString::operator=(const ETString &other)
{
    data = other.data;
    return *this;
}

ETString &ETString::operator=(unsigned char *s)
{
    data = (char *)s;
    return *this;
}

ETString &ETString::operator=(const char *s)
{
    data = s;
    return *this;
}
ETString &ETString::operator=(const std::string &s)
{
    data = s.c_str();
    return *this;
}

#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
ETString::ETString(const String &s) : data(s) {}
ETString::ETString(const std::string &s) : data(s.c_str()) {}
ETString &ETString::operator=(const String &s)
{
    data = s;
    return *this;
}
ETString::operator String() const { return data; }
ETString::operator std::string() const { return std::string(data.c_str()); }
#else
ETString::ETString(const std::string &s) : data(s) {}
ETString::operator std::string() const { return data; }
#endif

size_t ETString::length() const { return data.length(); }
bool ETString::empty() const { return data.length() == 0; }
char ETString::operator[](size_t idx) const { return data[idx]; }
char &ETString::operator[](size_t idx) { return data[idx]; }

ETString ETString::substr(size_t pos) const
{
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
    return data.substring(pos);
#else
    return data.substr(pos);
#endif
}
ETString ETString::substr(size_t pos, size_t len) const
{
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
    return data.substring(pos, pos + len);
#else
    return data.substr(pos, len);
#endif
}

size_t ETString::find(const ETString &str) const
{
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
    return data.indexOf(str.data);
#else
    return data.find(str.data);
#endif
}
size_t ETString::find(const ETString &str, size_t pos) const
{
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
    return data.indexOf(str.data, pos);
#else
    return data.find(str.data, pos);
#endif
}

size_t ETString::find(char c) const
{
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
    return data.indexOf(c);
#else
    return data.find(c);
#endif
}
size_t ETString::find(char c, size_t pos) const
{
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
    return data.indexOf(c, pos);
#else
    return data.find(c, pos);
#endif
}

size_t ETString::find_last_of(char c) const
{
    auto pos = data.length() - 1;
    return find_last_of(c, pos);
}

bool ETString::endsWith(char c) const
{
    return data[data.length() - 1] == c;
}
bool ETString::endsWith(const ETString &suffix) const
{
    size_t strLen = length();
    size_t suffixLen = suffix.length();

    if (suffixLen > strLen)
    {
        return false;
    }

    // Compare from the tail of data
    for (size_t i = 0; i < suffixLen; ++i)
    {
        if (data[strLen - suffixLen + i] != suffix.data[i])
        {
            return false;
        }
    }
    return true;
}

bool ETString::startsWith(char c) const
{
    return data[0] == c;
}
bool ETString::startsWith(const ETString &prefix) const
{
    size_t preLen = prefix.length();
    size_t strLen = length();

    // If prefix is longer than our string, it can't match
    if (preLen > strLen)
    {
        return false;
    }

    // Compare each character
    for (size_t i = 0; i < preLen; ++i)
    {
        if (data[i] != prefix.data[i])
        {
            return false;
        }
    }
    return true;
}

size_t ETString::find_last_of(char c, size_t pos) const
{
    if (pos > data.length() - 1)
    {
        pos = data.length() - 1;
    }
    for (int i = pos; i >= 0; --i)
        if (data[i] == c)
            return i;
    return npos;
}
size_t ETString::find_last_of(const ETString &search) const
{
    return find_last_of(search, length() == 0 ? 0 : length() - 1);
}

size_t ETString::find_last_of(const ETString &search, size_t pos) const
{
    if (search.length() == 0 || length() == 0)
        return npos;
#ifdef ARDUINO
    int end = pos < (length() - 1) ? pos : length() - 1;
    int found = data.lastIndexOf(search.data, end);
    return found >= 0 ? (size_t)found : npos;
#else
    return data.rfind(search.data, pos);
#endif
}

void ETString::erase(size_t pos, size_t len)
{
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
    data.remove(pos, len);
#else
    data.erase(pos, len);
#endif
}
void ETString::insert(size_t pos, char c)
{
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
    data = data.substring(0, pos) + String(c) + data.substring(pos);
#else
    data = data.substr(0, pos) + c + data.substr(pos);
#endif
}
void ETString::remove(size_t pos)
{
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
    data.remove(pos);
#else
    data.erase(pos, 1);
#endif
}
void ETString::toLowerCase()
{
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
    data.toLowerCase();
#else
    for (auto &c : data)
        c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
#endif
}
void ETString::pop_back()
{
    if (data.length() > 0)
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
        data.remove(data.length() - 1);
#else
        data.erase(data.length() - 1, 1);
#endif
}
void ETString::push_back(char c)
{
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
    data += c;
#else
    data.push_back(c);
#endif
}

bool ETString::operator==(const ETString &other) const { return data == other.data; }
bool ETString::operator!=(const ETString &other) const { return data != other.data; }
ETString ETString::operator+(const ETString &other) const { return data + other.data; }
ETString &ETString::operator+=(const ETString &other)
{
    data += other.data;
    return *this;
}
ETString &ETString::operator+=(const char c)
{
    data += c;
    return *this;
}
ETString ETString::trim() const
{
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
    auto tmp = data;
    tmp.trim();
    return tmp;
#else
    size_t start = 0;
    size_t end = data.length();
    while (start < end && isspace(static_cast<unsigned char>(data[start])))
        ++start;
    while (end > start && isspace(static_cast<unsigned char>(data[end - 1])))
        --end;
    return data.substr(start, end - start);
#endif
}

ETString ETString::cleanupString() const
{
    ETString cleaned;
    size_t cursor = 0;
    size_t len = data.length();
    for (size_t i = 0; i < len; ++i)
    {
        char c = data[i];
        // Backspace
        if (c == '\b')
        {
            if (cursor > 0)
            {
                cleaned.remove(cursor - 1);
                --cursor;
            }
        }
        // Left arrow (ASCII 0x1B 0x5B 0x44)
        else if (c == 0x1B && i + 2 < len && data[i + 1] == '[' && data[i + 2] == 'D')
        {
            if (cursor > 0)
                --cursor;
            i += 2;
        }
        // Right arrow (ASCII 0x1B 0x5B 0x43)
        else if (c == 0x1B && i + 2 < len && data[i + 1] == '[' && data[i + 2] == 'C')
        {
            if (cursor < cleaned.length())
                ++cursor;
            i += 2;
        }
        // Home (ASCII 0x1B 0x5B 0x48)
        else if (c == 0x1B && i + 2 < len && data[i + 1] == '[' && data[i + 2] == 'H')
        {
            cursor = 0;
            i += 2;
        }
        // End (ASCII 0x1B 0x5B 0x46)
        else if (c == 0x1B && i + 2 < len && data[i + 1] == '[' && data[i + 2] == 'F')
        {
            cursor = cleaned.length();
            i += 2;
        }
        // Printable ASCII (space to tilde)
        else if (c >= 32 && c <= 126)
        {
            cleaned.insert(cursor, c);
            ++cursor;
        }
        // Ignore other control characters
    }
    return cleaned;
}

void ETString::reserve(size_t bytes)
{
    data.reserve(bytes);
}
char ETString::back() const
{
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
    return data[data.length() - 1];
#else
    return data.back();
#endif
}

const char *ETString::c_str() const { return data.c_str(); }

bool ETString::operator<(const ETString &other) const { return data < other.data; }
bool ETString::operator>(const ETString &other) const { return data > other.data; }
bool ETString::operator<=(const ETString &other) const { return data <= other.data; }
bool ETString::operator>=(const ETString &other) const { return data >= other.data; }

bool ETString::contains(const ETString &other) const
{
    return find(other) != npos;
}

bool ETString::contains(const ETString &other, size_t pos) const
{
    return find(other, pos) != npos;
}

bool ETString::contains(const char *other) const
{
    return find(other) != npos;
}

bool ETString::contains(const char other) const
{
    return find(other) != npos;
}

size_t ETString::toull(const char *str, size_t *idx, int base)
{
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
    char *endPtr;
    unsigned long result = strtoul(str, &endPtr, base);
    if (idx)
        *idx = endPtr - str;
    return result;
#else
    size_t processedChars = 0;
    unsigned long result = std::stoul(str, &processedChars, base);
    if (idx)
        *idx = processedChars;
    return result;
#endif
}

template <typename... Args>
ETString string_format(const ETString &format, Args... args)
{
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
    if (size_s <= 0)
        throw std::runtime_error("Error during formatting.");
    auto size = static_cast<size_t>(size_s);
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args...);
    return ETString(std::string(buf.get(), buf.get() + size - 1));
}

ETVector<ETString> split(ETString s, ETString delimiter)
{
    ETVector<ETString> tokens;
    size_t pos = 0;
    while ((pos = s.find(delimiter)) != std::string::npos)
    {
        tokens.push_back(s.substr(0, pos));
        s.erase(0, pos + delimiter.length());
    }
    if (!s.empty())
        tokens.push_back(s);
    return tokens;
}

ETVector<ETString> sort(const ETVector<ETString> &input)
{
    ETVector<ETString> sorted = input;
    std::sort(sorted.begin(), sorted.end());

    return sorted;
}

ETString join(const ETVector<ETString> &elements, const ETString &delimiter)
{
    if (elements.empty())
        return ETString("");
    ETString result = elements[0];
    for (size_t i = 1; i < elements.size(); ++i)
        result += delimiter + elements[i];
    return result;
}

ETString toETString(size_t src)
{
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
    return ETString(String(src));
#else
    return ETString(std::to_string(src));
#endif
}

ETString toETString(unsigned long src)
{
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
    return ETString(String(src));
#else
    return ETString(std::to_string(src));
#endif
}

ETString toETString(int src)
{
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
    return ETString(String(src));
#else
    return ETString(std::to_string(src));
#endif
}