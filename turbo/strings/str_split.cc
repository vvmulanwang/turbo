// Copyright 2017 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "turbo/strings/str_split.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <limits>
#include <memory>

#include "turbo/base/internal/raw_logging.h"
#include "turbo/strings/ascii.h"

namespace turbo {
TURBO_NAMESPACE_BEGIN

namespace {

// This GenericFind() template function encapsulates the finding algorithm
// shared between the ByString and ByAnyChar delimiters. The FindPolicy
// template parameter allows each delimiter to customize the actual find
// function to use and the length of the found delimiter. For example, the
// Literal delimiter will ultimately use turbo::string_view::find(), and the
// AnyOf delimiter will use turbo::string_view::find_first_of().
template <typename FindPolicy>
turbo::string_view GenericFind(turbo::string_view text,
                              turbo::string_view delimiter, size_t pos,
                              FindPolicy find_policy) {
  if (delimiter.empty() && text.length() > 0) {
    // Special case for empty string delimiters: always return a zero-length
    // turbo::string_view referring to the item at position 1 past pos.
    return turbo::string_view(text.data() + pos + 1, 0);
  }
  size_t found_pos = turbo::string_view::npos;
  turbo::string_view found(text.data() + text.size(),
                          0);  // By default, not found
  found_pos = find_policy.Find(text, delimiter, pos);
  if (found_pos != turbo::string_view::npos) {
    found = turbo::string_view(text.data() + found_pos,
                              find_policy.Length(delimiter));
  }
  return found;
}

// Finds using turbo::string_view::find(), therefore the length of the found
// delimiter is delimiter.length().
struct LiteralPolicy {
  size_t Find(turbo::string_view text, turbo::string_view delimiter, size_t pos) {
    return text.find(delimiter, pos);
  }
  size_t Length(turbo::string_view delimiter) { return delimiter.length(); }
};

// Finds using turbo::string_view::find_first_of(), therefore the length of the
// found delimiter is 1.
struct AnyOfPolicy {
  size_t Find(turbo::string_view text, turbo::string_view delimiter, size_t pos) {
    return text.find_first_of(delimiter, pos);
  }
  size_t Length(turbo::string_view /* delimiter */) { return 1; }
};

}  // namespace

//
// ByString
//

ByString::ByString(turbo::string_view sp) : delimiter_(sp) {}

turbo::string_view ByString::Find(turbo::string_view text, size_t pos) const {
  if (delimiter_.length() == 1) {
    // Much faster to call find on a single character than on an
    // turbo::string_view.
    size_t found_pos = text.find(delimiter_[0], pos);
    if (found_pos == turbo::string_view::npos)
      return turbo::string_view(text.data() + text.size(), 0);
    return text.substr(found_pos, 1);
  }
  return GenericFind(text, delimiter_, pos, LiteralPolicy());
}

//
// ByChar
//

turbo::string_view ByChar::Find(turbo::string_view text, size_t pos) const {
  size_t found_pos = text.find(c_, pos);
  if (found_pos == turbo::string_view::npos)
    return turbo::string_view(text.data() + text.size(), 0);
  return text.substr(found_pos, 1);
}

//
// ByAnyChar
//

ByAnyChar::ByAnyChar(turbo::string_view sp) : delimiters_(sp) {}

turbo::string_view ByAnyChar::Find(turbo::string_view text, size_t pos) const {
  return GenericFind(text, delimiters_, pos, AnyOfPolicy());
}

//
// ByLength
//
ByLength::ByLength(ptrdiff_t length) : length_(length) {
  TURBO_RAW_CHECK(length > 0, "");
}

turbo::string_view ByLength::Find(turbo::string_view text,
                                      size_t pos) const {
  pos = std::min(pos, text.size());  // truncate `pos`
  turbo::string_view substr = text.substr(pos);
  // If the string is shorter than the chunk size we say we
  // "can't find the delimiter" so this will be the last chunk.
  if (substr.length() <= static_cast<size_t>(length_))
    return turbo::string_view(text.data() + text.size(), 0);

  return turbo::string_view(substr.data() + length_, 0);
}

TURBO_NAMESPACE_END
}  // namespace turbo
