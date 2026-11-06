// Part of the Reflex project.
// Copyright © Reflex Project Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <concepts>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <reflex/function_traits.hpp>

#include "test_types.hpp"

namespace reflex {

namespace {

#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wundefined-internal"
#elif defined __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-function"
#endif

void test_function_1();
int test_function_2(float) noexcept;
const std::string &&test_function_3(int, float *const &, const std::vector<void *> &);

struct TestClass {
    void member_fn_1();
    int member_fn_2(std::string &&);
    std::tuple<std::string, float> member_fn_3(int, const std::vector<float> &) const;
};

#ifdef __clang__
    #pragma clang diagnostic pop
#elif defined __GNUC__
    #pragma GCC diagnostic pop
#endif

} // namespace

TEST_CASE("function_traits contains correct information for functions") {
    using traits_1 = function_traits<test_function_1>;
    STATIC_REQUIRE(traits_1::is_noexcept == false);
    STATIC_REQUIRE(std::same_as<traits_1::result_type, void>);
    STATIC_REQUIRE(std::same_as<traits_1::argument_types, boost::mp11::mp_list<>>);

    using traits_2 = function_traits<test_function_2>;
    STATIC_REQUIRE(traits_2::is_noexcept == true);
    STATIC_REQUIRE(std::same_as<traits_2::result_type, int>);
    STATIC_REQUIRE(std::same_as<traits_2::argument_types, boost::mp11::mp_list<float>>);

    using traits_3 = function_traits<test_function_3>;
    STATIC_REQUIRE(traits_3::is_noexcept == false);
    STATIC_REQUIRE(std::same_as<traits_3::result_type, const std::string &&>);
    STATIC_REQUIRE(std::same_as<traits_3::argument_types,
                                boost::mp11::mp_list<int, float *const &, const std::vector<void *> &>>);
}

TEST_CASE("member_function_traits contains correct information for member functions") {
    using traits_1 = member_function_traits<&TestClass::member_fn_1>;
    STATIC_REQUIRE(std::same_as<traits_1::class_type, TestClass>);
    STATIC_REQUIRE(std::same_as<traits_1::self_type, TestClass>);
    STATIC_REQUIRE(std::same_as<traits_1::result_type, void>);
    STATIC_REQUIRE(std::same_as<traits_1::argument_types, boost::mp11::mp_list<>>);

    using traits_2 = member_function_traits<&TestClass::member_fn_2>;
    STATIC_REQUIRE(std::same_as<traits_2::class_type, TestClass>);
    STATIC_REQUIRE(std::same_as<traits_2::self_type, TestClass>);
    STATIC_REQUIRE(std::same_as<traits_2::result_type, int>);
    STATIC_REQUIRE(std::same_as<traits_2::argument_types, boost::mp11::mp_list<std::string &&>>);

    using traits_3 = member_function_traits<&TestClass::member_fn_3>;
    STATIC_REQUIRE(std::same_as<traits_3::class_type, TestClass>);
    STATIC_REQUIRE(std::same_as<traits_3::self_type, const TestClass>);
    STATIC_REQUIRE(std::same_as<traits_3::result_type, std::tuple<std::string, float>>);
    STATIC_REQUIRE(std::same_as<traits_3::argument_types, boost::mp11::mp_list<int, const std::vector<float> &>>);
}

TEST_CASE("func_meta is available for function with return type containing dependent template ") {
    using meta_t = func_meta<tests::dependent_result_function>;
    STATIC_REQUIRE(meta_t::name == "dependent_result_function");
}

} // namespace reflex
