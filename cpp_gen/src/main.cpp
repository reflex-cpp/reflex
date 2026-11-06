// Part of the Reflex project.
// Copyright © Reflex Project Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

#include <boost/json/parse.hpp>
#include <fmt/format.h>
#include <fmt/os.h>
#if FMT_VERSION >= 90000 // 9.0.0
    #include <fmt/std.h> // std::filesystem::path formatter
#else
    #include <fmt/ostream.h>
#endif

#include "generate.hpp"
#include "metadata.hpp"

namespace reflex::cpp_gen {
namespace {

std::string load_file_to_string(const std::filesystem::path &path) {
    auto file = std::ifstream{path, std::ios::in | std::ios::binary};
    if (!file) {
        throw std::runtime_error{fmt::format("Failed to open {}", path)};
    }
    if (!file.seekg(0, std::ios::end)) {
        throw std::runtime_error{fmt::format("Failed to seek to the end of {}", path)};
    }
    const auto file_size = file.tellg();
    if (file_size == -1) {
        throw std::runtime_error{fmt::format("Failed to get the size of {}", path)};
    }
    if (!file.seekg(0, std::ios::beg)) {
        throw std::runtime_error{fmt::format("Failed to seek to the beginning of {}", path)};
    }
    auto str = std::string(static_cast<std::size_t>(file_size), char{});
    if (!file.read(str.data(), file_size)) {
        throw std::runtime_error{fmt::format("Failed to read from {}", path)};
    }
    return str;
}

[[nodiscard]] int run(const int argc, const char *const *const argv) try {
    if (argc != 3) {
        fmt::print(stderr,
                   "Incorrect usage. Expected \"{}\" <metadata_path> <header_path>",
                   argc == 0 ? "reflex_cpp_gen" : argv[0]);
        return EXIT_FAILURE;
    }
    const auto metadata_path = std::filesystem::path{argv[1]};
    const auto header_path = std::filesystem::path{argv[2]};

    const auto json_str = load_file_to_string(metadata_path);
    const auto json = boost::json::parse(json_str,
                                         {},
                                         {.max_depth = 256, .numbers = boost::json::number_precision::precise});
    const auto metadata = boost::json::value_to<Metadata>(json);

    {
        auto header_file = fmt::buffered_file{header_path.string(), "wb"};
        generate_header(metadata, header_file);
    }
    return EXIT_SUCCESS;
} catch (const std::exception &ex) {
    fmt::print(stderr, "Fatal error: {}\n", ex.what());
    return EXIT_FAILURE;
} catch (...) {
    fmt::print(stderr, "Fatal error: <unknown exception>\n");
    return EXIT_FAILURE;
}

} // namespace
} // namespace reflex::cpp_gen

int main(const int argc, const char *const *const argv) { return reflex::cpp_gen::run(argc, argv); }
