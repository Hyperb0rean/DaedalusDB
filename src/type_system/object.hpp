#pragma once

#include <cstddef>
#include <string>

#include "class.hpp"
#include "file.hpp"

#define DDB_PRIMITIVE_GENERATOR(MACRO) \
    MACRO(int)                         \
    MACRO(double)                      \
    MACRO(float)                       \
    MACRO(bool)                        \
    MACRO(unsigned int)                \
    MACRO(short int)                   \
    MACRO(short unsigned int)          \
    MACRO(long long int)               \
    MACRO(long long unsigned int)      \
    MACRO(long unsigned int)           \
    MACRO(long int)                    \
    MACRO(char)                        \
    MACRO(signed char)                 \
    MACRO(unsigned char)               \
    MACRO(unsigned long)               \
    MACRO(wchar_t)

namespace ts {

class Object {
protected:
    Class::Ptr class_;

public:
    using Ptr = util::Ptr<Object>;

    [[nodiscard]] virtual Class::Ptr GetClass() {
        return class_;
    }
    virtual ~Object() = default;
    [[nodiscard]] virtual size_t Size() const = 0;
    virtual mem::Offset Write(mem::File::Ptr& file, mem::Offset offset) const = 0;
    virtual void Read(mem::File::Ptr& file, mem::Offset offset) = 0;
    [[nodiscard]] virtual std::string ToString() const = 0;
};

template <typename O>
concept ObjectLike = std::derived_from<O, Object>;

}  // namespace ts