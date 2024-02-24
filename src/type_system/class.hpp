#pragma once
#include <optional>

#include "utils.hpp"

namespace ts {

// WARN: Is compiler dependent
template <typename T>
[[nodiscard]] constexpr std::string_view type_name() {
    constexpr auto prefix = std::string_view{"[T = "};
    constexpr auto suffix = std::string_view{"]"};
    constexpr auto function = std::string_view{__PRETTY_FUNCTION__};
    constexpr auto start = function.find(prefix) + prefix.size();
    constexpr auto end = function.rfind(suffix);
    static_assert(start < end);
    constexpr auto result = function.substr(start, (end - start));
    return result;
}

class Class {
protected:
    std::string name_;

    // Is it necessary in Class definition ?
    void Validate() const {
        for (auto& c : name_) {
            if (c == '@' || c == '_' || c == '<' || c == '>') {
                throw error::TypeError("Invalid class name");
            }
        }
    }

public:
    using Ptr = util::Ptr<Class>;

    explicit Class(std::string name) : name_(std::move(name)) {
        Validate();
    }
    virtual ~Class() = default;

    [[nodiscard]] virtual std::string Serialize() const = 0;

    [[nodiscard]] virtual std::optional<size_t> Size() const = 0;

    [[nodiscard]] virtual std::string Name() const {
        return name_;
    }
    [[nodiscard]] virtual size_t Count() const = 0;
};

template <typename C>
concept ClassLike = std::derived_from<C, Class>;
///////////////////////////////////////////////////////////////////////
// Some inspiration for Type system realization from Yuko
///////////////////////////////////////////////////////////////////////

/*
template <class T>
typedef T* (*supplier)();

class MyFieldInfo {
protected:
    std::string fname;
    int size;
    int fieldOffset;

public:
    MyFieldInfo(std::string&& name, int size, int fieldOffset) {
        this->name = name;
        this->size = size;
        this->fieldOffset = fieldOffset;
    }

    void rawSetValue(T* obj, void* value) {
        memcpy(obj + fieldOffset, value, size);
    }

    void rawGetValue(T* obj, void* value) {
        memcpy(value, obj + fieldOffset, size);
    }
};

template <class T>
class MyTypeFieldInfoBase : public MyFieldInfo {
    MyTypeFieldInfoBase(std::string&& name, int size, int fieldOffset)
        : MyFieldInfo(name, size, fieldOffset) {
    }
}

template <class T, class F>
class MyTypeFieldInfo : public MyTypeFieldInfoBase<T> {
private:
    F T::*fieldRelPtr;

public:
    typedef F T::*fieldRelPtrType;

    MyTypeFieldInfo(std::string&& fname, fieldRelPtrType fieldRelPtr)
        : MyTypeFieldInfoBase(name, sizeof(F), (int)fieldRelPtr) {
        this->fieldRelPtr = fieldRelPtr;
    }

    void setValue(T* obj, F value) {
        obj.*fieldRelPtr = value;
    }

    F getValue(T* obj) {
        return obj.*fieldRelPtr;
    }
};

class MyTypeInfoBase {
protected:
    std::string tname;
    std::vector<MyFieldInfo> fields;

public:
    MyTypeInfoBase(std::string&& tname, MyTypeFieldInfoBase<T>... fields) {
        this->tname = tname;
        for (auto&& f : {fields...}) {
            this->fields.push_back(f);
        }
    }

    virtual void* createInstance() = 0;
};

template <class T>
class MyTypeInfo : public MyTypeInfoBase {
private:
    std::string tname;
    supplier<T> ctor;
    std::vector<MyTypeFieldInfoBase<T>> fields;

public:
    MyTypeInfo(std::string&& tname, supplier<T> ctor, MyTypeFieldInfoBase<T>... fields)
        : MyTypeInfoBase(name, fields) {
        this->tname = tname;
        this->ctor = ctor;
        for (auto&& f : {fields...}) {
            this->fields.push_back(f);
        }
    }

    void* createInstance() override {
        return this->ctor();
    }
};

#define F(fname, fref) MyTypeFieldInfo(fname, fref)
#define F(fref) MyTypeFieldInfo(#fref, fref)

typedef struct {
    float x, y, z;
} Vec3f;

typedef struct {
    Vec3f a, b;
} Rectf;

void test() {

    MyTypeInfo<Vec3f> vt("ttt", []() -> new Vec3f(), F(&Vec3f::x), F(&Vec3f::y), F(&Vec3f::z));

    iterator<T> it;

    while (it.next()) {
        T obj;
        it.fill(obj);
    }
}
*/

///////////////////////////////////////////////////////////////////////

}  // namespace ts