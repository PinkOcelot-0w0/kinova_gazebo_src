// Copyright 2024 AIT - Austrian Institute of Technology GmbH
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef CONTROL_FILTERS__CUSTOM_VALIDATORS_HPP_
#define CONTROL_FILTERS__CUSTOM_VALIDATORS_HPP_

#include <fmt/core.h>

#include <string>

#include <rclcpp/rclcpp.hpp>
#include <rsl/parameter_validators.hpp>
#include <tl_expected/expected.hpp>

namespace control_filters
{

/**
 * @brief gt_eq, but check only if the value is not NaN
 */
template <typename T>
tl::expected<void, std::string> gt_eq_or_nan(rclcpp::Parameter const & parameter, T expected_value)
{
  auto param_value = parameter.as_double();
  if (!std::isnan(param_value))
  {
    // check only if the value is not NaN
    return rsl::gt_eq<T>(parameter, expected_value);
  }
  return {};
}

/**
 * @brief lt_eq, but check only if the value is not NaN
 */
template <typename T>
tl::expected<void, std::string> lt_eq_or_nan(rclcpp::Parameter const & parameter, T expected_value)
{
  auto param_value = parameter.as_double();
  if (!std::isnan(param_value))
  {
    // check only if the value is not NaN
    return rsl::lt_eq<T>(parameter, expected_value);
  }
  return {};
}

}  // namespace control_filters

#endif  // CONTROL_FILTERS__CUSTOM_VALIDATORS_HPP_
