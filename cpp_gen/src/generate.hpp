// Part of the Reflex project.
// Copyright © Reflex Project Contributors.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <fmt/os.h>

#include "metadata.hpp"

namespace reflex::cpp_gen {

void generate_header(const Metadata &metadata, fmt::buffered_file &file);

}
