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

    [[nodiscard]] auto GetClass() noexcept -> Class::Ptr {
        return class_;
    }
    virtual ~Object() = default;
    [[nodiscard]] virtual auto Size() const -> size_t = 0;
    virtual auto Write(mem::File::Ptr& file, mem::Offset offset) const -> mem::Offset = 0;
    virtual auto Read(mem::File::Ptr& file, mem::Offset offset) -> void = 0;
    [[nodiscard]] virtual auto ToString() const -> std::string = 0;
};

template <typename O>
concept ObjectLike = std::derived_from<O, Object>;

}  // namespace ts