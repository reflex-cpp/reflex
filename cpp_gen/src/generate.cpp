// Part of the Reflex project.
// Copyright © Reflex Project Contributors.
// SPDX-License-Identifier: Apache-2.0

#include "generate.hpp"

#include <functional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/algorithm/string/replace.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>

namespace reflex::cpp_gen {

namespace {

using namespace std::string_view_literals;
using namespace std::string_literals;

[[nodiscard]] std::string remove_namespace(const std::string_view full_name, const std::string_view _namespace) {
    if (_namespace.empty()) {
        return std::string{full_name};
    }
    return std::string{full_name.substr(_namespace.size() + 2)}; // also remove "::"
}

[[nodiscard]] std::string metadata_struct_name(auto &data) {
    auto clean_type_name = remove_namespace(data.full_name, data._namespace);
    boost::algorithm::replace_all(clean_type_name, "::"sv, "_"sv);
    return fmt::format("meta_{}", clean_type_name);
}

[[nodiscard]] std::string metadata_namespace(const std::string_view _namespace) {
    if (_namespace.empty()) {
        return "_reflex_detail";
    }
    return fmt::format("{}::_reflex_detail", _namespace);
}

[[nodiscard]] std::string format_template_parameters(const std::span<const TemplateArgument> parameters) {
    return fmt::format("{}",
                       fmt::join(parameters | std::views::transform([](const TemplateArgument &parameter) {
                                     if (parameter.kind == TemplateArgumentKind::type) {
                                         return fmt::format("typename {}", parameter.name);
                                     }
                                     return fmt::format("auto {}", parameter.name);
                                 }),
                                 ", "));
}

[[nodiscard]] std::string format_template_argument_list(const std::span<const TemplateArgument> parameters) {
    return fmt::format("<{}>", fmt::join(parameters | std::views::transform(&TemplateArgument::name), ", "));
}

[[nodiscard]] std::string format_properties(const std::span<const Property> properties) {
    return fmt::format("reflex::tuple{{{}}}",
                       fmt::join(properties | std::views::transform(&Property::expression), ", "));
}

[[nodiscard]] std::string format_default_argument(const FunctionArgument &parameter) {
    if (!parameter.default_value) {
        return "nullptr";
    }
    return fmt::format("[]() -> {} {{ return {}; }}", parameter.type.full_name, *parameter.default_value);
}

[[nodiscard]] std::string format_function_parameter_info(const FunctionArgument &parameter) {
    auto property_types = fmt::join(parameter.properties | std::views::transform([](const Property &property) {
                                        return fmt::format("std::decay_t<decltype({})>", property.expression);
                                    }),
                                    ", ");
    return fmt::format("reflex::function_parameter_info<{}{}{}>{{\"{}\", {}, {}}}",
                       parameter.type.full_name,
                       parameter.properties.empty() ? ""sv : ", "sv,
                       std::move(property_types),
                       parameter.name,
                       format_default_argument(parameter),
                       format_properties(parameter.properties));
}

void generate_enum_metadata_struct(const Enumeration &enum_meta, fmt::buffered_file &file) {
    const auto no_enumerators_have_properties = std::ranges::all_of(
            enum_meta.enumerators,
            [](const Enumerator &enumerator) { return enumerator.properties.empty(); });

    const auto enumerator_tuple_type = [&] {
        if (no_enumerators_have_properties) {
            const auto enumerator_info_type = fmt::format("reflex::enumerator_info<{}>", enum_meta.full_name);
            const auto enumerator_info_type_list = std::vector(enum_meta.enumerators.size(),
                                                               std::string_view{enumerator_info_type});
            return fmt::format("reflex::tuple<{}>", fmt::join(enumerator_info_type_list, ", "));
        }
        return "reflex::tuple"s;
    }();

    const auto format_enumerator_metadata = [&](const Enumerator &enumerator) {
        const auto type_name = enumerator.properties.empty()
                                       ? fmt::format("reflex::enumerator_info<{}>", enum_meta.full_name)
                                       : "reflex::enumerator_info";
        return fmt::format("{0}{{{1}::{2}, \"{2}\", {3}}}",
                           type_name,
                           enum_meta.full_name,
                           enumerator.name,
                           format_properties(enumerator.properties));
    };

    file.print(R"(
namespace {0} {{

struct {1} {{
    static constexpr auto enumerators = {2}{{{3}}};
    static constexpr auto properties = {4};
}};

}} // namespace {0}
)",
               metadata_namespace(enum_meta._namespace),
               metadata_struct_name(enum_meta),
               enumerator_tuple_type,
               fmt::join(enum_meta.enumerators | std::views::transform(format_enumerator_metadata), ",\n"),
               format_properties(enum_meta.properties));
}

[[nodiscard]] std::string format_struct_member_initializer(const DataMember &member_meta) {
    if (!member_meta.initializer) {
        return "nullptr";
    }
    const auto &initializer = *member_meta.initializer;
    switch (initializer.style) {
        case MemberInitializerStyle::copy:
            return fmt::format("[]() -> {} {{ return {}; }}", member_meta.type.full_name, initializer.expression);
        case MemberInitializerStyle::list:
            return fmt::format("[] {{ return {}{}; }}", member_meta.type.full_name, initializer.expression);
    }
    throw std::runtime_error{"unexpected member initializer style"};
}

void generate_struct_metadata_struct(const Structure &struct_meta, fmt::buffered_file &file) {
    const auto struct_template_arguments = struct_meta.template_info
                                                   ? format_template_argument_list(struct_meta.template_info->arguments)
                                                   : std::string{};
    const auto format_data_member_info = [&](const DataMember &member) {
        return fmt::format("reflex::data_member_info{{&{0}{1}::{2}, \"{2}\", {3}, {4}}}",
                           struct_meta.full_name,
                           struct_template_arguments,
                           member.name,
                           format_struct_member_initializer(member),
                           format_properties(member.properties));
    };

    const auto format_template_str = [&](const TemplateInfo &template_info) {
        return fmt::format("template<{}>\n", format_template_parameters(template_info.arguments));
    };
    const auto template_str = struct_meta.template_info ? format_template_str(*struct_meta.template_info)
                                                        : std::string{};

    const auto format_member_function_info = [&](const Method &member_function, const Structure &struct_meta) {
        const auto struct_template_arguments = struct_meta.template_info ? format_template_argument_list(
                                                                                   struct_meta.template_info->arguments)
                                                                         : std::string{};

        return fmt::format(
                R"(
{6}
struct {0}_{1} {{
    static constexpr auto name = std::string_view{{"{1}"}};
    static constexpr auto full_name = std::string_view{{"{2}::{1}"}};
    static constexpr auto ptr = &{2}{3}::{1};
    static constexpr auto parameters = reflex::tuple{{{4}}};
    static constexpr auto properties = {5};
}};
)",
                metadata_struct_name(struct_meta),
                member_function.name,
                struct_meta.full_name,
                struct_template_arguments,
                fmt::join(member_function.arguments | std::views::transform(format_function_parameter_info), ", "),
                format_properties(member_function.properties),
                template_str);
    };

    file.print(R"(
namespace {0} {{

{1}

{2}struct {3} {{
    static constexpr auto data_members = reflex::tuple{{{4}}};
    static constexpr auto properties = {5};
    using member_functions_t = boost::mp11::mp_list<{6}>;
}};

}} // {0}
)",
               metadata_namespace(struct_meta._namespace),
               fmt::join(struct_meta.methods | std::views::filter([](const Method &method) {
                             return method.access == MemberAccess::_public;
                         }) |
                                 std::views::transform(
                                         [&format_member_function_info, &struct_meta](const Method &member_function) {
                                             return format_member_function_info(member_function, struct_meta);
                                         }),
                         "\n"),
               template_str,
               metadata_struct_name(struct_meta),
               fmt::join(struct_meta.data_members | std::views::filter([](const DataMember &meta_member) {
                             return meta_member.access == MemberAccess::_public;
                         }) | std::views::transform(format_data_member_info),
                         ", "),
               format_properties(struct_meta.properties),
               fmt::join(struct_meta.methods | std::views::filter([](const Method &method) {
                             return method.access == MemberAccess::_public;
                         }) |
                                 std::views::transform(
                                         [&struct_meta, &struct_template_arguments](const Method &member_function) {
                                             return fmt::format("{}_{}{}",
                                                                metadata_struct_name(struct_meta),
                                                                member_function.name,
                                                                struct_template_arguments);
                                         }),
                         ", "));
}

void generate_function_metadata_struct(const FunctionInfo &function_meta, fmt::buffered_file &file) {
    file.print(R"(
namespace {0} {{

struct {1} {{
    static constexpr auto name = std::string_view{{"{2}"}};
    static constexpr auto full_name = std::string_view{{"{3}"}};
    static constexpr auto parameters = reflex::tuple{{{4}}};
    static constexpr auto properties = {5};
}};

}} // namespace {0}
)",
               metadata_namespace(function_meta._namespace),
               metadata_struct_name(function_meta),
               function_meta.name,
               function_meta.full_name,
               fmt::join(function_meta.arguments | std::views::transform(format_function_parameter_info), ", "),
               format_properties(function_meta.properties));
}

void generate_meta_specialization(const auto &data, fmt::buffered_file &file) {
    const auto *const template_info = [&]() -> const TemplateInfo * {
        if constexpr (requires { data.template_info; }) {
            return data.template_info ? &*data.template_info : nullptr;
        } else {
            return nullptr;
        }
    }();
    const auto template_parameters = template_info ? format_template_parameters(template_info->arguments)
                                                   : std::string{};
    const auto template_arguments = template_info ? format_template_argument_list(template_info->arguments)
                                                  : std::string{};
    file.print(R"(
template<{0}>
struct meta<{1}{2}> : ::{3}::{4}{2} {{}};
)",
               template_parameters,
               data.full_name,
               template_arguments,
               metadata_namespace(data._namespace),
               metadata_struct_name(data));
}

void generate_member_func_meta_specialization(const Structure &struct_data,
                                              const Method &method_data,
                                              fmt::buffered_file &file) {
    const auto &template_info = struct_data.template_info;

    const auto template_parameters = template_info ? format_template_parameters(template_info->arguments)
                                                   : std::string{};
    const auto template_arguments = template_info ? format_template_argument_list(template_info->arguments)
                                                  : std::string{};
    if (method_data.access == MemberAccess::_public && !method_data.is_inherited && !template_info.has_value()) {
        file.print(R"(
template<{0}>
struct member_func_meta<&{1}{2}::{3}> : ::{4}::{5}_{3}{2} {{}};
)",
                   template_parameters,
                   struct_data.full_name,
                   template_arguments,
                   method_data.name,
                   metadata_namespace(struct_data._namespace),
                   metadata_struct_name(struct_data));
    }
}


void generate_func_meta_specialization(const FunctionInfo &function_meta, fmt::buffered_file &file) {
    file.print(R"(
template<>
struct func_meta<{}> : ::{}::{} {{}};
)",
               function_meta.full_name,
               metadata_namespace(function_meta._namespace),
               metadata_struct_name(function_meta));
}

} // namespace

void generate_header(const Metadata &metadata, fmt::buffered_file &file) {
    const auto unique_structures = [&] {
        return metadata.structures | std::views::filter(std::not_fn(&Structure::is_template_instantiation));
    };

    file.print(R"(#pragma once

// WARNING: This file is generated by reflex_cpp_gen. Do not edit manually!

#include <array>
#include <type_traits>
#include <string_view>

#include <boost/mp11/list.hpp>

#include <reflex/meta.hpp>
#include <reflex/tuple.hpp>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces"
#endif
)");

    for (const auto &enum_meta : metadata.enumerations) {
        generate_enum_metadata_struct(enum_meta, file);
    }
    for (const auto &struct_meta : unique_structures()) {
        generate_struct_metadata_struct(struct_meta, file);
    }
    for (const auto &function_meta : metadata.functions) {
        generate_function_metadata_struct(function_meta, file);
    }

    file.print(R"(
namespace reflex {{
)");

    for (const auto &enum_meta : metadata.enumerations) {
        generate_meta_specialization(enum_meta, file);
    }
    for (const auto &struct_meta : unique_structures()) {
        generate_meta_specialization(struct_meta, file);
        for (const auto &member_func_meta : struct_meta.methods) {
            generate_member_func_meta_specialization(struct_meta, member_func_meta, file);
        }
    }
    for (const auto &function_meta : metadata.functions) {
        generate_func_meta_specialization(function_meta, file);
    }

    file.print(R"(
}} // namespace reflex

#ifdef __clang__
#pragma clang diagnostic pop
#endif

)");
}

} // namespace reflex::cpp_gen