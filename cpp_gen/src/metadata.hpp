// Part of the Reflex project.
// Copyright © Reflex Project Contributors.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include <boost/describe/class.hpp>
#include <boost/describe/enum.hpp>
#include <boost/json/conversion.hpp>
#include <boost/json/error.hpp>
#include <boost/json/value.hpp>
#include <boost/json/value_to.hpp>
#include <boost/system/system_error.hpp>

namespace reflex::cpp_gen {

struct TypeInformation;
using IndirectTypeInformation = std::unique_ptr<TypeInformation>;

template<typename BaseStruct>
struct WithNamespaceMixin : BaseStruct {
    std::string _namespace;

    friend WithNamespaceMixin tag_invoke(const boost::json::value_to_tag<WithNamespaceMixin> &,
                                         const boost::json::value &value) {
        const auto &obj = value.as_object();
        const auto &ns_str = obj.at("namespace").as_string();
        auto copy = value;
        copy.get_object().erase("namespace");
        return WithNamespaceMixin{boost::json::value_to<BaseStruct>(copy), std::string{ns_str}};
    }
};


enum class TemplateArgumentKind { integral, type, expression };

BOOST_DESCRIBE_ENUM(TemplateArgumentKind, integral, type, expression)

struct IntegralTemplateArgumentValue {
    double value;
};

BOOST_DESCRIBE_STRUCT(IntegralTemplateArgumentValue, (), (value))

struct IntegralTemplateArgumentInfo {
    TemplateArgumentKind kind; // integral
    IntegralTemplateArgumentValue integral;
};

BOOST_DESCRIBE_STRUCT(IntegralTemplateArgumentInfo, (), (kind, integral))

struct TypeTemplateParameterInfo {
    TemplateArgumentKind kind; // type
    IndirectTypeInformation type;
};

BOOST_DESCRIBE_STRUCT(TypeTemplateParameterInfo, (), (kind, type))

struct ExpressionTemplateParameterInfo {
    TemplateArgumentKind kind; // expression
};
BOOST_DESCRIBE_STRUCT(ExpressionTemplateParameterInfo, (), (kind))

using TemplateArgumentInfo =
        std::variant<IntegralTemplateArgumentInfo, TypeTemplateParameterInfo, ExpressionTemplateParameterInfo>;

struct TemplateSpecializationInfo {
    std::string template_name;
    std::vector<TemplateArgumentInfo> arguments;
};

BOOST_DESCRIBE_STRUCT(TemplateSpecializationInfo, (), (template_name, arguments))

struct TypeAliasInfo {
    IndirectTypeInformation referenced_type;
};

BOOST_DESCRIBE_STRUCT(TypeAliasInfo, (), (referenced_type))

struct TypeInformation {
    std::string name;
    std::string full_name;
    bool is_const;
    bool is_volatile;
    std::optional<TemplateSpecializationInfo> template_info;
    std::optional<TypeAliasInfo> alias_info;
};

BOOST_DESCRIBE_STRUCT(TypeInformation, (), (name, full_name, is_const, is_volatile, template_info, alias_info))

inline IndirectTypeInformation tag_invoke(const boost::json::value_to_tag<IndirectTypeInformation> &,
                                          const boost::json::value &value) {
    return std::make_unique<TypeInformation>(boost::json::value_to<TypeInformation>(value));
}

struct PropertyValue {
    std::string value;
};

BOOST_DESCRIBE_STRUCT(PropertyValue, (), (value))

struct Property {
    std::string expression;
    TypeInformation type;
    std::vector<PropertyValue> values;
};

BOOST_DESCRIBE_STRUCT(Property, (), (expression, type, values))

struct FunctionArgument {
    std::string name;
    TypeInformation type;
    std::optional<std::string> default_value;
    std::vector<Property> properties;
};

BOOST_DESCRIBE_STRUCT(FunctionArgument, (), (name, type, default_value, properties))

enum class MemberAccess {
    _public,
    _private,
    _protected,
};

inline MemberAccess tag_invoke(const boost::json::value_to_tag<MemberAccess> &, const boost::json::value &value) {
    const auto &str = value.as_string();
    if (str == "public") {
        return MemberAccess::_public;
    }
    if (str == "private") {
        return MemberAccess::_private;
    }
    if (str == "protected") {
        return MemberAccess::_protected;
    }
    throw boost::system::system_error{boost::json::error::unknown_name};
}

struct Method {
    std::string name;
    MemberAccess access;

    bool is_static;
    bool is_const;

    bool is_inherited;

    TypeInformation return_type;
    std::vector<FunctionArgument> arguments;

    std::vector<Property> properties;
};

BOOST_DESCRIBE_STRUCT(Method, (), (name, access, is_static, is_const, is_inherited, return_type, arguments, properties))

struct TemplateArgument {
    std::string name;
    TemplateArgumentKind kind;
};

BOOST_DESCRIBE_STRUCT(TemplateArgument, (), (name, kind))

struct TemplateInfo {
    std::vector<TemplateArgument> arguments;
};

BOOST_DESCRIBE_STRUCT(TemplateInfo, (), (arguments))

enum class MemberInitializerStyle {
    copy,
    list,
};

BOOST_DESCRIBE_ENUM(MemberInitializerStyle, copy, list)

struct MemberInitializer {
    MemberInitializerStyle style;
    std::string expression;
};

BOOST_DESCRIBE_STRUCT(MemberInitializer, (), (style, expression))

struct DataMember {
    std::string name;
    TypeInformation type;
    MemberAccess access;
    std::vector<Property> properties;
    std::optional<MemberInitializer> initializer;
};

BOOST_DESCRIBE_STRUCT(DataMember, (), (name, type, access, properties, initializer))

struct StructureBase {
    std::string name;
    // std::string namespace;
    std::string full_name;
    std::vector<DataMember> data_members;
    std::vector<Method> methods;
    std::vector<Property> properties;

    bool is_template_instantiation;
    std::optional<TemplateInfo> template_info;
    std::optional<TemplateSpecializationInfo> template_specialization_info;
};

BOOST_DESCRIBE_STRUCT(StructureBase,
                      (),
                      (name,
                       full_name,
                       data_members,
                       methods,
                       properties,
                       is_template_instantiation,
                       template_info,
                       template_specialization_info))

using Structure = WithNamespaceMixin<StructureBase>;

struct Enumerator {
    std::string name;
    std::variant<std::string, double> value;
    std::vector<Property> properties;
};

BOOST_DESCRIBE_STRUCT(Enumerator, (), (name, value, properties))

struct EnumerationBase {
    std::string name;
    // std::string namespace;
    std::string full_name;
    std::vector<Enumerator> enumerators;
    std::vector<Property> properties;
};

BOOST_DESCRIBE_STRUCT(EnumerationBase, (), (name, full_name, enumerators, properties))

using Enumeration = WithNamespaceMixin<EnumerationBase>;

struct FunctionInfoBase {
    std::string name;
    // std::string namespace;
    std::string full_name;
    TypeInformation return_type;

    std::vector<FunctionArgument> arguments;

    std::vector<Property> properties;
};

BOOST_DESCRIBE_STRUCT(FunctionInfoBase, (), (name, full_name, return_type, arguments, properties))

using FunctionInfo = WithNamespaceMixin<FunctionInfoBase>;

struct Metadata {
    std::vector<Enumeration> enumerations;
    std::vector<Structure> structures;
    std::vector<FunctionInfo> functions;
};

BOOST_DESCRIBE_STRUCT(Metadata, (), (enumerations, structures, functions))

} // namespace reflex::cpp_gen
