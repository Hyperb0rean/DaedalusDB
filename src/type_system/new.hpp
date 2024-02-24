#pragma once

#include <any>
#include <initializer_list>
#include <string>

#include "primitive.hpp"
#include "string.hpp"
#include "struct.hpp"

namespace ts {

// Should be twice size of mem::PageOffset;
using ObjectId = uint64_t;

template <ClassLike C, typename... Classes>
[[nodiscard]] util::Ptr<C> NewClass(std::string&& name, Classes&&... classes) {
    if constexpr (std::is_same_v<C, StructClass>) {
        auto new_class = util::MakePtr<C>(std::move(name));
        (new_class->AddField(classes), ...);
        return new_class;
    } else {
        static_assert(sizeof...(classes) == 0);
        return util::MakePtr<C>(std::move(name));
    }
}

template <ObjectLike O, ClassLike C>
[[nodiscard]] util::Ptr<O> UnsafeNew(util::Ptr<C> object_class,
                                     std::initializer_list<std::any>::iterator& arg_it) {
    if constexpr (std::is_same_v<O, Struct>) {
        auto new_object = util::MakePtr<Struct>(object_class);
        auto it = util::As<StructClass>(object_class)->GetFields().begin();
        auto end = util::As<StructClass>(object_class)->GetFields().end();
        for (; it != end; ++it) {
            if (util::Is<StructClass>(*it)) {
                new_object->AddFieldValue(UnsafeNew<Struct>(util::As<StructClass>(*it), arg_it));
            } else if (util::Is<StringClass>(*it)) {
                if (arg_it->type() == typeid(std::string)) {
                    new_object->AddFieldValue(
                        util::MakePtr<String>(util::As<StringClass>(*it),
                                              std::move(std::any_cast<std::string>(*arg_it))));
                } else if (arg_it->type() == typeid(const char*)) {
                    new_object->AddFieldValue(
                        util::MakePtr<String>(util::As<StringClass>(*it),
                                              std::move(std::any_cast<const char*>(*arg_it))));
                } else {
                    throw error::TypeError(
                        "Incorrect cast or attempt to implicit type conversion of argument type " +
                        std::string(arg_it->type().name()) + " to class type" +
                        util::As<StringClass>(*it)->Serialize());
                }
                ++arg_it;
            } else {

#define DDB_ADD_PRIMITIVE(P)                                                                \
    if (util::Is<PrimitiveClass<P>>(*it)) {                                                 \
        if (arg_it->type() == typeid(P)) {                                                  \
            new_object->AddFieldValue(util::MakePtr<Primitive<P>>(                          \
                util::As<PrimitiveClass<P>>(*it), std::any_cast<P>(*arg_it++)));            \
            continue;                                                                       \
        } else                                                                              \
            throw error::TypeError(                                                         \
                "Incorrect cast or attempt to implicit type conversion of argument type " + \
                std::string(arg_it->type().name()) + " to class type" +                     \
                util::As<PrimitiveClass<P>>(*it)->Serialize());                             \
    }

                DDB_PRIMITIVE_GENERATOR(DDB_ADD_PRIMITIVE)
#undef DDB_ADD_PRIMITIVE
            }
        }

        return new_object;
    } else if constexpr (std::is_same_v<O, String>) {
        if (arg_it->type() == typeid(std::string)) {
            return util::MakePtr<String>(object_class,
                                         std::move(std::any_cast<std::string>(*arg_it)));
        } else if (arg_it->type() == typeid(const char*)) {
            return util::MakePtr<String>(object_class,
                                         std::move(std::any_cast<const char*>(*arg_it)));
        }
        throw error::TypeError(
            "Incorrect cast or attempt to implicit type conversion of argument type " +
            std::string(arg_it->type().name()) + " to class type" + object_class->Serialize());
    } else {
#define DDB_ADD_PRIMITIVE(P)                                                                \
    if (util::Is<PrimitiveClass<P>>(object_class)) {                                        \
        if (arg_it->type() == typeid(P)) {                                                  \
            return util::MakePtr<Primitive<P>>(util::As<PrimitiveClass<P>>(object_class),   \
                                               std::any_cast<P>(*arg_it));                  \
        } else                                                                              \
            throw error::TypeError(                                                         \
                "Incorrect cast or attempt to implicit type conversion of argument type " + \
                std::string(arg_it->type().name()) + " to class type" +                     \
                object_class->Serialize());                                                 \
    }

        DDB_PRIMITIVE_GENERATOR(DDB_ADD_PRIMITIVE)
#undef DDB_ADD_PRIMITIVE

        throw error::TypeError("Can't create object for type " + object_class->Name());
    }
}

template <ObjectLike O, ClassLike C, typename... Args>
[[nodiscard]] util::Ptr<O> New(util::Ptr<C> object_class, Args&&... args) {

    if (object_class->Count() != sizeof...(Args)) {
        throw error::BadArgument("Wrong number of arguments");
    }
    std::initializer_list<std::any> list = {args...};
    std::initializer_list<std::any>::iterator it = list.begin();
    return UnsafeNew<O>(object_class, it);
}

template <ObjectLike O, ClassLike C>
[[nodiscard]] util::Ptr<O> DefaultNew(util::Ptr<C> object_class) {

    if constexpr (std::is_same_v<O, Struct>) {
        auto new_object = util::MakePtr<Struct>(object_class);
        auto fields = util::As<StructClass>(object_class)->GetFields();
        for (auto it = fields.begin(); it != fields.end(); ++it) {
            if (util::Is<StructClass>(*it)) {
                new_object->AddFieldValue(DefaultNew<Struct>(util::As<StructClass>(*it)));
            } else if (util::Is<StringClass>(*it)) {
                new_object->AddFieldValue(DefaultNew<String>(util::As<StringClass>(*it)));
            } else {
#define DDB_ADD_PRIMITIVE(P)                                                                   \
    if (util::Is<PrimitiveClass<P>>(*it)) {                                                    \
        new_object->AddFieldValue(DefaultNew<Primitive<P>>(util::As<PrimitiveClass<P>>(*it))); \
        continue;                                                                              \
    }
                DDB_PRIMITIVE_GENERATOR(DDB_ADD_PRIMITIVE)
#undef DDB_ADD_PRIMITIVE

                throw error::TypeError("Class can't be defaulted");
            }
        }
        return new_object;
    } else if constexpr (std::is_same_v<O, String>) {
        return util::MakePtr<String>(object_class);
    } else {
        return util::MakePtr<O>(object_class);
    }
    throw error::TypeError("Can't create object");
}

template <ObjectLike O, ClassLike C>
[[nodiscard]] util::Ptr<O> ReadNew(util::Ptr<C> object_class, mem::File::Ptr& file,
                                   mem::Offset offset) {

    auto new_object = DefaultNew<O, C>(object_class);
    new_object->Read(file, offset);
    return new_object;
}
}  // namespace ts