// Copyright (c) 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/base/location.h"

#if defined(_WIN32)
#include <intrin.h>
#endif

#include <sstream>

#include "platform/base/compiler_specific.h"

namespace openscreen {

Location::Location() = default;
Location::Location(const Location&) = default;
Location::Location(Location&&) noexcept = default;

Location::Location(const void* program_counter)
    : program_counter_(program_counter) {}

Location& Location::operator=(const Location& other) = default;
Location& Location::operator=(Location&& other) = default;

std::string Location::ToString() const {
  if (program_counter_ == nullptr) {
    return "pc:nullptr";
  }

  std::ostringstream oss;
  oss << "pc:0x" << std::hex << reinterpret_cast<uintptr_t>(program_counter_);
  return oss.str();
}

#if defined(__GNUC__) || defined(__clang__)
#define RETURN_ADDRESS() \
  __builtin_extract_return_addr(__builtin_return_address(0))
#elif defined(_MSC_VER)
#define RETURN_ADDRESS() _ReturnAddress()
#else
#define RETURN_ADDRESS() nullptr
#endif

// static
OSP_NOINLINE Location Location::CreateFromHere() {
  return Location(RETURN_ADDRESS());
}

// static
OSP_NOINLINE const void* GetProgramCounter() {
  return RETURN_ADDRESS();
}

std::ostream& operator<<(std::ostream& out, const Location& location) {
  return out << location.ToString();
}

}  // namespace openscreen
