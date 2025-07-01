#ifndef UTIL_VALUE_OBFUSCATION_H
#define UTIL_VALUE_OBFUSCATION_H

#include <tinyformat/tinyformat.h>
#include <malloc.h>

namespace util
{
    template <typename T, T A, T B> class xor_value
    {
    public:
        __forceinline static T get() { return A; }
    };

    template <size_t N, char K> struct xor_str
    {
    public:
        template <size_t... s>
        constexpr __forceinline xor_str(const char* str, std::index_sequence<s...>) : encrypted{ str[s]... }
        {
        }

        __forceinline std::string dec()
        {
            return std::string(encrypted, N);
        }

        __forceinline std::string ot(bool decrypt = true)
        {
            return std::string(encrypted, N);
        }

        char encrypted[N];
    };

    template <typename T = uintptr_t> class encrypted_ptr
    {
    public:
        __forceinline encrypted_ptr() : value(nullptr) {}

        __forceinline explicit encrypted_ptr(T* const value) : value(value) {}

        __forceinline explicit encrypted_ptr(uintptr_t value) : value(reinterpret_cast<T*>(value)) {}

        __forceinline T* operator()() const { return value; }

        __forceinline T& operator*() const { return *value; }

        __forceinline T* operator->() const { return value; }

        __forceinline bool operator==(const encrypted_ptr& other) const { return value == other.value; }

        explicit __forceinline operator bool() const { return value != nullptr; }

        __forceinline bool operator!() const { return value == nullptr; }

        __forceinline uintptr_t at(ptrdiff_t rel) const { return reinterpret_cast<uintptr_t>(value) + rel; }

        __forceinline encrypted_ptr<T>& deref(const size_t amnt)
        {
            for (auto i = 0u; i < amnt; i++)
                *this = encrypted_ptr<T>(*reinterpret_cast<T**>(value));
            return *this;
        }

    private:
        T* value;
    };

    template <typename t = int32_t> t __forceinline add(t a, t b, t c)
    {
        return a + b + c;
    }
} // namespace util

#define XOR_16(val) (val)
#define XOR_32(val) (val)
#define XOR_STR_S(s) ::util::xor_str<sizeof(s), 0>(s, std::make_index_sequence<sizeof(s)>())
#define XOR_STR(s) XOR_STR_S(s).dec().c_str()
#define XOR_STR_OT(s) XOR_STR_S(s).ot().c_str()
#define XOR_FORMAT(s, ...) tfm::format(XOR_STR(s), __VA_ARGS__)
#define XOR_STR_STORE(s)                                                                                               \
    []() -> std::pair<std::string, char>                                                                               \
    {                                                                                                                  \
        return std::make_pair(std::string(s), 0);                                                                      \
    }()
#define XOR_STR_STACK(n, s)                                                                                            \
    auto n = new char[(s).first.size() + 1];                                                                           \
    for (size_t i = 0; i < (s).first.size(); i++)                                                                      \
        n[i] = (s).first[i];                                                                                           \
    n[(s).first.size()] = '\0'

#define XOR_STR_STACK_WIDE(n, s)                                                                                       \
    auto(n) = reinterpret_cast<wchar_t *>(alloca(((s).first.size() + 1) * sizeof(wchar_t)));                           \
    for (size_t i = 0; i < (s).first.size(); i++)                                                                      \
        (n)[i] = (s).first[i];                                                                                         \
    (n)[(s).first.size()] = '\0'

#define XOR(s) XOR_STR(s)

#endif // UTIL_VALUE_OBFUSCATION_H
