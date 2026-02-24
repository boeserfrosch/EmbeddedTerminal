#ifndef ET_TYPES_H
#define ET_TYPES_H

#if defined(ARDUINO)
#include <Arduino.h>
#endif
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
#include <ArxContainer.h>
#else
#include <string>
#include <vector>
#include <map>
#endif
#include <stdexcept>
#include <memory>

class ETString
{
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
    String data;
#else
    std::string data;
#endif
public:
    ETString();
    ETString(const char *s);
    ETString(const unsigned char *s);
    ETString(const std::string &s);
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
    ETString(const String &s);
#endif
    ETString(const ETString &other);
    ETString &operator=(const ETString &other);
    ETString &operator=(const char *s);
    ETString &operator=(char *s);
    ETString &operator=(unsigned char *s);
    ETString &operator=(const std::string &s);
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
    ETString &operator=(const String &s);
#endif
    // Conversion
    operator std::string() const;
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
    operator String() const;
#endif
    // Methods
    size_t length() const;
    bool empty() const;
    char operator[](size_t idx) const;
    char &operator[](size_t idx);
    ETString substr(size_t pos, size_t len) const;
    ETString substr(size_t pos) const;
    size_t find(const ETString &str, size_t pos) const;
    size_t find(const ETString &str) const;
    size_t find(char c, size_t pos) const;
    size_t find(char c) const;
    size_t find_last_of(char c, size_t pos) const;
    size_t find_last_of(char c) const;
    size_t find_last_of(const ETString &search) const;
    size_t find_last_of(const ETString &search, size_t pos) const;
    bool endsWith(char suffix) const;
    bool endsWith(const ETString &suffix) const;
    bool startsWith(char prefix) const;
    bool startsWith(const ETString &prefix) const;
    void erase(size_t pos, size_t len = npos);
    void insert(size_t pos, char c);
    void remove(size_t pos);
    void toLowerCase();
    void pop_back();
    void push_back(char c);
    bool operator==(const ETString &other) const;
    bool operator!=(const ETString &other) const;
    ETString operator+(const ETString &other) const;
    ETString &operator+=(const ETString &other);
    ETString &operator+=(char c);
    ETString trim() const;
    ETString cleanupString() const;
    void reserve(size_t);
    char back() const;
    // C-string
    const char *c_str() const;
    bool operator<(const ETString &other) const;
    bool operator>(const ETString &other) const;
    bool operator<=(const ETString &other) const;
    bool operator>=(const ETString &other) const;
    bool contains(const ETString &other) const;
    bool contains(const ETString &other, size_t pos) const;
    bool contains(const char *other) const;
    bool contains(const char other) const;

    static const size_t npos =
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
        -1;
#else
        std::string::npos;
#endif

    static size_t toull(const char *str, size_t *idx = nullptr, int base = 10);
};

inline ETString operator+(const char *lhs, const ETString &rhs)
{
    ETString tmp(lhs); // build ETString from "abc"
    tmp += rhs;        // append the RHS
    return tmp;        // return by value
}

template <typename T>
using ETVector = std::vector<T>;
template <typename K, typename V>
using ETMap = std::map<K, V>;

// ETString trim(const ETString &str);
// ETString cleanupLine(const ETString &line);

template <typename... Args>
ETString string_format(const ETString &format, Args... args);

void toLower(ETString &data);

ETVector<ETString> split(ETString s, ETString delimiter);
ETString join(const ETVector<ETString> &elements, const ETString &delimiter);

ETString toETString(size_t src);
ETString toETString(int src);
ETString toETString(unsigned long src);

#endif // ET_TYPES_H
