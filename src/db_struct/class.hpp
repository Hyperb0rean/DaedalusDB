#pragma once
#include <sstream>
#include <typeinfo>

#include "file.hpp"
#include "utils.hpp"

namespace ts {

// TODO: May be compiler dependent
template <typename T>
constexpr std::string_view type_name() {
    constexpr auto prefix = std::string_view{"[with T = "};
    constexpr auto suffix = std::string_view{";"};
    constexpr auto function = std::string_view{__PRETTY_FUNCTION__};
    constexpr auto start = function.find(prefix) + prefix.size();
    constexpr auto end = function.rfind(suffix);
    static_assert(start < end);
    constexpr auto result = function.substr(start, (end - start));
    return result;
}

struct Class {
    std::string name_;
    virtual ~Class() = default;
    [[nodiscard]] virtual std::string Serialize() const = 0;
};
template <typename T>
struct PrimitiveClass : public Class {
    explicit PrimitiveClass(std::string name) {
        this->name_ = name;
    }
    [[nodiscard]] std::string Serialize() const override {
        std::string result = "_";
        result += type_name<T>();
        result += "@";
        result += name_;
        result += "_";
        return {result.begin(), std::remove_if(result.begin(), result.end(), isspace)};
        // return result;
    }
};

template <typename C>
concept ClassLike = std::derived_from<C, Class>;

struct StringClass : public Class {
    explicit StringClass(std::string name) {
        this->name_ = name;
    }
    [[nodiscard]] std::string Serialize() const override {
        return "_string@" + name_ + "_";
    }
};

struct StructClass : public Class {
    std::vector<std::shared_ptr<Class>> fields_;

    StructClass(std::string name) {
        this->name_ = name;
    }
    template <ClassLike ActualClass>
    void AddField(ActualClass field) {
        fields_.push_back(std::make_shared<ActualClass>(field));
    }

    template <ClassLike ActualClass>
    void AddField(const std::shared_ptr<ActualClass>& field) {
        fields_.push_back(field);
    }

    [[nodiscard]] std::string Serialize() const override {
        auto result = "_struct@" + name_ + "_<";
        for (auto& field : fields_) {
            result += field->Serialize();
        }
        result += ">";
        return result;
    }
};

template <ClassLike C, typename... Classes>
[[nodiscard]] inline std::shared_ptr<C> NewClass(std::string&& name, Classes&&... classes) {
    if constexpr (std::is_same_v<C, StructClass>) {
        auto new_class = std::make_shared<C>(std::move(name));
        (new_class->AddField(classes), ...);
        return new_class;
    } else {
        static_assert(sizeof...(classes) == 0);
        return std::make_shared<C>(std::move(name));
    }
}

}  // namespace ts