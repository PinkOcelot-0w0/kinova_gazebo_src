// Copyright (c) 2023, Stogl Robotics Consulting UG (haftungsbeschränkt)
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
#include <memory>
#include <string>

#include "geometry_msgs/msg/wrench_stamped.hpp"
#include "gmock/gmock.h"
#include "pluginlib/class_loader.hpp"
#include "rclcpp/utilities.hpp"

#include "control_filters/low_pass_filter.hpp"

TEST(TestLoadLowPassFilter, load_low_pass_filter_double)
{
  rclcpp::init(0, nullptr);

  pluginlib::ClassLoader<filters::FilterBase<double>> filter_loader(
    "filters", "filters::FilterBase<double>");
  std::shared_ptr<filters::FilterBase<double>> filter;
  auto available_classes = filter_loader.getDeclaredClasses();
  std::stringstream sstr;
  sstr << "available filters:" << std::endl;
  for (const auto & available_class : available_classes)
  {
    sstr << "  " << available_class << std::endl;
  }

  std::string filter_type = "control_filters/LowPassFilterDouble";
  ASSERT_TRUE(filter_loader.isClassAvailable(filter_type)) << sstr.str();
  EXPECT_NO_THROW(filter = filter_loader.createSharedInstance(filter_type));

  rclcpp::shutdown();
}

TEST(TestLoadLowPassFilter, load_low_pass_filter_vector_double)
{
  rclcpp::init(0, nullptr);

  pluginlib::ClassLoader<filters::FilterBase<std::vector<double>>> filter_loader(
    "filters", "filters::FilterBase<std::vector<double>>");
  std::shared_ptr<filters::FilterBase<std::vector<double>>> filter;
  auto available_classes = filter_loader.getDeclaredClasses();
  std::stringstream sstr;
  sstr << "available filters:" << std::endl;
  for (const auto & available_class : available_classes)
  {
    sstr << "  " << available_class << std::endl;
  }

  std::string filter_type = "control_filters/LowPassFilterVectorDouble";
  ASSERT_TRUE(filter_loader.isClassAvailable(filter_type)) << sstr.str();
  EXPECT_NO_THROW(filter = filter_loader.createSharedInstance(filter_type));

  rclcpp::shutdown();
}

TEST(TestLoadLowPassFilter, load_low_pass_filter_wrench)
{
  rclcpp::init(0, nullptr);

  pluginlib::ClassLoader<filters::FilterBase<geometry_msgs::msg::WrenchStamped>> filter_loader(
    "filters", "filters::FilterBase<geometry_msgs::msg::WrenchStamped>");
  std::shared_ptr<filters::FilterBase<geometry_msgs::msg::WrenchStamped>> filter;
  auto available_classes = filter_loader.getDeclaredClasses();
  std::stringstream sstr;
  sstr << "available filters:" << std::endl;
  for (const auto & available_class : available_classes)
  {
    sstr << "  " << available_class << std::endl;
  }

  std::string filter_type = "control_filters/LowPassFilterWrench";
  ASSERT_TRUE(filter_loader.isClassAvailable(filter_type)) << sstr.str();
  EXPECT_NO_THROW(filter = filter_loader.createSharedInstance(filter_type));

  rclcpp::shutdown();
}
