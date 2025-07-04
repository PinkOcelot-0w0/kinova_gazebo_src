// Copyright (c) 2020, Open Source Robotics Foundation, Inc.
// All rights reserved.
//
// Software License Agreement (BSD License 2.0)
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above
//    copyright notice, this list of conditions and the following
//    disclaimer in the documentation and/or other materials provided
//    with the distribution.
//  * Neither the name of the Willow Garage nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING INtry_publish
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "control_toolbox/pid_ros.hpp"

namespace control_toolbox
{
constexpr double UMAX_INFINITY = std::numeric_limits<double>::infinity();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
PidROS::PidROS(
  rclcpp::node_interfaces::NodeBaseInterface::SharedPtr node_base,
  rclcpp::node_interfaces::NodeLoggingInterface::SharedPtr node_logging,
  rclcpp::node_interfaces::NodeParametersInterface::SharedPtr node_params,
  rclcpp::node_interfaces::NodeTopicsInterface::SharedPtr topics_interface, std::string prefix,
  bool prefix_is_for_params)
: node_base_(node_base),
  node_logging_(node_logging),
  node_params_(node_params),
  topics_interface_(topics_interface)
{
  if (prefix_is_for_params)
  {
    param_prefix_ = prefix;
    // If it starts with a "~", remove it
    if (param_prefix_.compare(0, 1, "~") == 0)
    {
      param_prefix_.erase(0, 1);
    }
    // If it starts with a "/" or a "~/", remove those as well
    if (param_prefix_.compare(0, 1, "/") == 0)
    {
      param_prefix_.erase(0, 1);
    }
    // Add a trailing "."
    if (!param_prefix_.empty() && param_prefix_.back() != '.')
    {
      param_prefix_.append(".");
    }

    topic_prefix_ = prefix;
    // Replace parameter separator from "." to "/" in topics
    std::replace(topic_prefix_.begin(), topic_prefix_.end(), '.', '/');
    // Add a trailing "/"
    if (!topic_prefix_.empty() && topic_prefix_.back() != '/')
    {
      topic_prefix_.append("/");
    }
    // Add global namespace if none is defined
    if (topic_prefix_.compare(0, 1, "~") != 0 && topic_prefix_.compare(0, 1, "/") != 0)
    {
      topic_prefix_ = "/" + topic_prefix_;
    }
  }
  else
  {
    set_prefixes(prefix);
  }

  state_pub_ = rclcpp::create_publisher<control_msgs::msg::PidState>(
    topics_interface_, topic_prefix_ + "pid_state", rclcpp::SensorDataQoS());
  rt_state_pub_.reset(
    new realtime_tools::RealtimePublisher<control_msgs::msg::PidState>(state_pub_));
}
#pragma GCC diagnostic pop

void PidROS::set_prefixes(const std::string & topic_prefix)
{
  param_prefix_ = topic_prefix;
  // If it starts with a "~", remove it
  if (param_prefix_.compare(0, 1, "~") == 0)
  {
    param_prefix_.erase(0, 1);
  }
  // If it starts with a "/" or a "~/", remove those as well
  if (param_prefix_.compare(0, 1, "/") == 0)
  {
    param_prefix_.erase(0, 1);
  }
  // Replace namespacing separator from "/" to "." in parameters
  std::replace(param_prefix_.begin(), param_prefix_.end(), '/', '.');
  // Add a trailing "."
  if (!param_prefix_.empty() && param_prefix_.back() != '.')
  {
    param_prefix_.append(".");
  }

  topic_prefix_ = topic_prefix;
  // Replace parameter separator from "." to "/" in topics
  std::replace(topic_prefix_.begin(), topic_prefix_.end(), '.', '/');
  // Add a trailing "/"
  if (!topic_prefix_.empty() && topic_prefix_.back() != '/')
  {
    topic_prefix_.append("/");
  }
}

bool PidROS::get_boolean_param(const std::string & param_name, bool & value)
{
  declare_param(param_name, rclcpp::ParameterValue(value));
  rclcpp::Parameter param;
  if (node_params_->has_parameter(param_name))
  {
    node_params_->get_parameter(param_name, param);
    if (rclcpp::PARAMETER_BOOL != param.get_type())
    {
      RCLCPP_ERROR(
        node_logging_->get_logger(), "Wrong parameter type '%s', not boolean", param_name.c_str());
      return false;
    }
    value = param.as_bool();
    return true;
  }
  else
  {
    return false;
  }
}

// TODO(anyone): to-be-removed once this functionality becomes supported by the param API directly
bool PidROS::get_double_param(const std::string & param_name, double & value)
{
  declare_param(param_name, rclcpp::ParameterValue(value));
  rclcpp::Parameter param;
  if (node_params_->has_parameter(param_name))
  {
    node_params_->get_parameter(param_name, param);
    if (rclcpp::PARAMETER_DOUBLE != param.get_type())
    {
      RCLCPP_ERROR(
        node_logging_->get_logger(), "Wrong parameter type '%s', not double", param_name.c_str());
      return false;
    }
    value = param.as_double();
    RCLCPP_DEBUG_STREAM(
      node_logging_->get_logger(), "parameter '" << param_name << "' in node '"
                                                 << node_base_->get_name() << "' value is " << value
                                                 << std::endl);
    return true;
  }
  else
  {
    RCLCPP_ERROR_STREAM(
      node_logging_->get_logger(), "parameter '" << param_name << "' in node '"
                                                 << node_base_->get_name() << "' does not exists"
                                                 << std::endl);
    return false;
  }
}

bool PidROS::get_string_param(const std::string & param_name, std::string & value)
{
  declare_param(param_name, rclcpp::ParameterValue(value));
  rclcpp::Parameter param;
  if (node_params_->has_parameter(param_name))
  {
    node_params_->get_parameter(param_name, param);
    if (rclcpp::PARAMETER_STRING != param.get_type())
    {
      RCLCPP_ERROR(
        node_logging_->get_logger(), "Wrong parameter type '%s', not string", param_name.c_str());
      return false;
    }
    value = param.as_string();
    RCLCPP_DEBUG_STREAM(
      node_logging_->get_logger(), "parameter '" << param_name << "' in node '"
                                                 << node_base_->get_name() << "' value is " << value
                                                 << std::endl);
    return true;
  }
  else
  {
    RCLCPP_ERROR_STREAM(
      node_logging_->get_logger(), "parameter '" << param_name << "' in node '"
                                                 << node_base_->get_name() << "' does not exists"
                                                 << std::endl);
    return false;
  }
}

bool PidROS::initialize_from_ros_parameters()
{
  double p, i, d, i_max, i_min, u_max, u_min, trk_tc;
  p = i = d = i_max = i_min = trk_tc = std::numeric_limits<double>::quiet_NaN();
  u_max = UMAX_INFINITY;
  u_min = -UMAX_INFINITY;
  bool antiwindup = false;
  std::string antiwindup_strat_str = "none";
  bool all_params_available = true;

  all_params_available &= get_double_param(param_prefix_ + "p", p);
  all_params_available &= get_double_param(param_prefix_ + "i", i);
  all_params_available &= get_double_param(param_prefix_ + "d", d);
  all_params_available &= get_double_param(param_prefix_ + "i_clamp_max", i_max);
  all_params_available &= get_double_param(param_prefix_ + "i_clamp_min", i_min);
  all_params_available &= get_double_param(param_prefix_ + "u_clamp_max", u_max);
  all_params_available &= get_double_param(param_prefix_ + "u_clamp_min", u_min);
  all_params_available &= get_double_param(param_prefix_ + "tracking_time_constant", trk_tc);

  get_boolean_param(param_prefix_ + "antiwindup", antiwindup);
  get_string_param(param_prefix_ + "antiwindup_strategy", antiwindup_strat_str);
  declare_param(param_prefix_ + "save_i_term", rclcpp::ParameterValue(false));

  if (all_params_available)
  {
    set_parameter_event_callback();
  }

  if (antiwindup_strat_str == "none")
  {
    std::cout << "Old anti-windup technique is deprecated. "
                 "This option will be removed by the ROS 2 Kilted Kaiju release."
              << std::endl;
  }

  AntiwindupStrategy antiwindup_strat(antiwindup_strat_str);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  pid_.initialize(p, i, d, i_max, i_min, u_max, u_min, trk_tc, antiwindup, antiwindup_strat);
#pragma GCC diagnostic pop

  return all_params_available;
}

void PidROS::declare_param(const std::string & param_name, rclcpp::ParameterValue param_value)
{
  if (!node_params_->has_parameter(param_name))
  {
    node_params_->declare_parameter(param_name, param_value);
  }
}

void PidROS::initialize_from_args(
  double p, double i, double d, double i_max, double i_min, bool antiwindup)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  initialize_from_args(
    p, i, d, i_max, i_min, UMAX_INFINITY, -UMAX_INFINITY, 0.0, antiwindup, AntiwindupStrategy::NONE,
    false);
#pragma GCC diagnostic pop
}

void PidROS::initialize_from_args(
  double p, double i, double d, double i_max, double i_min, bool antiwindup, bool save_i_term)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  initialize_from_args(
    p, i, d, i_max, i_min, UMAX_INFINITY, -UMAX_INFINITY, 0.0, antiwindup, AntiwindupStrategy::NONE,
    save_i_term);
#pragma GCC diagnostic pop
}

void PidROS::initialize_from_args(
  double p, double i, double d, double i_max, double i_min, double u_max, double u_min,
  double trk_tc, bool antiwindup, AntiwindupStrategy antiwindup_strat, bool save_i_term)
{
  if (i_min > i_max)
  {
    RCLCPP_ERROR(node_logging_->get_logger(), "received i_min > i_max, skip new gains");
  }
  else if (u_min > u_max)
  {
    RCLCPP_ERROR(node_logging_->get_logger(), "received u_min > u_max, skip new gains");
  }
  else
  {
    if (antiwindup_strat == AntiwindupStrategy::NONE)
    {
      std::cout << "Old anti-windup technique is deprecated. "
                   "This option will be removed by the ROS 2 Kilted Kaiju release."
                << std::endl;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    pid_.initialize(p, i, d, i_max, i_min, u_max, u_min, trk_tc, antiwindup, antiwindup_strat);
#pragma GCC diagnostic pop
    const Pid::Gains gains = pid_.get_gains();

    declare_param(param_prefix_ + "p", rclcpp::ParameterValue(gains.p_gain_));
    declare_param(param_prefix_ + "i", rclcpp::ParameterValue(gains.i_gain_));
    declare_param(param_prefix_ + "d", rclcpp::ParameterValue(gains.d_gain_));
    declare_param(param_prefix_ + "i_clamp_max", rclcpp::ParameterValue(gains.i_max_));
    declare_param(param_prefix_ + "i_clamp_min", rclcpp::ParameterValue(gains.i_min_));
    declare_param(param_prefix_ + "u_clamp_max", rclcpp::ParameterValue(gains.u_max_));
    declare_param(param_prefix_ + "u_clamp_min", rclcpp::ParameterValue(gains.u_min_));
    declare_param(param_prefix_ + "tracking_time_constant", rclcpp::ParameterValue(gains.trk_tc_));
    declare_param(param_prefix_ + "saturation", rclcpp::ParameterValue(true));
    declare_param(param_prefix_ + "antiwindup", rclcpp::ParameterValue(gains.antiwindup_));
    declare_param(
      param_prefix_ + "antiwindup_strategy",
      rclcpp::ParameterValue(gains.antiwindup_strat_.to_string()));
    declare_param(param_prefix_ + "save_i_term", rclcpp::ParameterValue(save_i_term));

    set_parameter_event_callback();
  }
}

void PidROS::initialize_from_args(
  double p, double i, double d, double u_max, double u_min, double trk_tc,
  AntiwindupStrategy antiwindup_strat, bool save_i_term)
{
  if (u_min > u_max)
  {
    RCLCPP_ERROR(node_logging_->get_logger(), "received u_min > u_max, skip new gains");
  }
  else
  {
    pid_.initialize(p, i, d, u_max, u_min, trk_tc, antiwindup_strat);
    const Pid::Gains gains = pid_.get_gains();

    declare_param(param_prefix_ + "p", rclcpp::ParameterValue(gains.p_gain_));
    declare_param(param_prefix_ + "i", rclcpp::ParameterValue(gains.i_gain_));
    declare_param(param_prefix_ + "d", rclcpp::ParameterValue(gains.d_gain_));
    declare_param(param_prefix_ + "u_clamp_max", rclcpp::ParameterValue(gains.u_max_));
    declare_param(param_prefix_ + "u_clamp_min", rclcpp::ParameterValue(gains.u_min_));
    declare_param(param_prefix_ + "tracking_time_constant", rclcpp::ParameterValue(gains.trk_tc_));
    declare_param(param_prefix_ + "saturation", rclcpp::ParameterValue(true));
    declare_param(
      param_prefix_ + "antiwindup_strategy",
      rclcpp::ParameterValue(gains.antiwindup_strat_.to_string()));
    declare_param(param_prefix_ + "save_i_term", rclcpp::ParameterValue(save_i_term));

    set_parameter_event_callback();
  }
}

void PidROS::reset()
{
  bool save_i_term = false;
  get_boolean_param(param_prefix_ + "save_i_term", save_i_term);
  reset(save_i_term);
}

void PidROS::reset(bool save_i_term) { pid_.reset(save_i_term); }

std::shared_ptr<rclcpp::Publisher<control_msgs::msg::PidState>> PidROS::get_pid_state_publisher()
{
  return state_pub_;
}

double PidROS::compute_command(double error, const rclcpp::Duration & dt)
{
  double cmd = pid_.compute_command(error, dt);
  publish_pid_state(cmd, error, dt);
  return cmd;
}

double PidROS::compute_command(double error, double error_dot, const rclcpp::Duration & dt)
{
  double cmd = pid_.compute_command(error, error_dot, dt);
  publish_pid_state(cmd, error, dt);
  return cmd;
}

Pid::Gains PidROS::get_gains() { return pid_.get_gains(); }

void PidROS::set_gains(double p, double i, double d, double i_max, double i_min, bool antiwindup)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  set_gains(
    p, i, d, i_max, i_min, UMAX_INFINITY, -UMAX_INFINITY, 0.0, antiwindup,
    AntiwindupStrategy::NONE);
#pragma GCC diagnostic pop
}

void PidROS::set_gains(
  double p, double i, double d, double i_max, double i_min, double u_max, double u_min,
  double trk_tc, bool antiwindup, AntiwindupStrategy antiwindup_strat)
{
  if (i_min > i_max)
  {
    RCLCPP_ERROR(node_logging_->get_logger(), "received i_min > i_max, skip new gains");
  }
  else if (u_min > u_max)
  {
    RCLCPP_ERROR(node_logging_->get_logger(), "received u_min > u_max, skip new gains");
  }
  else
  {
    if (antiwindup_strat == AntiwindupStrategy::NONE)
    {
      std::cout << "Old anti-windup technique is deprecated. "
                   "This option will be removed by the ROS 2 Kilted Kaiju release."
                << std::endl;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    pid_.set_gains(p, i, d, i_max, i_min, u_max, u_min, trk_tc, antiwindup, antiwindup_strat);
#pragma GCC diagnostic pop
    const Pid::Gains gains = pid_.get_gains();

    node_params_->set_parameters(
      {rclcpp::Parameter(param_prefix_ + "p", gains.p_gain_),
       rclcpp::Parameter(param_prefix_ + "i", gains.i_gain_),
       rclcpp::Parameter(param_prefix_ + "d", gains.d_gain_),
       rclcpp::Parameter(param_prefix_ + "i_clamp_max", gains.i_max_),
       rclcpp::Parameter(param_prefix_ + "i_clamp_min", gains.i_min_),
       rclcpp::Parameter(param_prefix_ + "u_clamp_max", gains.u_max_),
       rclcpp::Parameter(param_prefix_ + "u_clamp_min", gains.u_min_),
       rclcpp::Parameter(param_prefix_ + "tracking_time_constant", gains.trk_tc_),
       rclcpp::Parameter(param_prefix_ + "saturation", true),
       rclcpp::Parameter(param_prefix_ + "antiwindup", gains.antiwindup_),
       rclcpp::Parameter(
         param_prefix_ + "antiwindup_strategy", gains.antiwindup_strat_.to_string())});
  }
}

void PidROS::set_gains(
  double p, double i, double d, double u_max, double u_min, double trk_tc,
  AntiwindupStrategy antiwindup_strat)
{
  if (u_min > u_max)
  {
    RCLCPP_ERROR(node_logging_->get_logger(), "received u_min > u_max, skip new gains");
  }
  else
  {
    pid_.set_gains(p, i, d, u_max, u_min, trk_tc, antiwindup_strat);
    const Pid::Gains gains = pid_.get_gains();
    node_params_->set_parameters(
      {rclcpp::Parameter(param_prefix_ + "p", gains.p_gain_),
       rclcpp::Parameter(param_prefix_ + "i", gains.i_gain_),
       rclcpp::Parameter(param_prefix_ + "d", gains.d_gain_),
       rclcpp::Parameter(param_prefix_ + "u_clamp_max", gains.u_max_),
       rclcpp::Parameter(param_prefix_ + "u_clamp_min", gains.u_min_),
       rclcpp::Parameter(param_prefix_ + "tracking_time_constant", gains.trk_tc_),
       rclcpp::Parameter(param_prefix_ + "saturation", true),
       rclcpp::Parameter(
         param_prefix_ + "antiwindup_strategy", gains.antiwindup_strat_.to_string())});
  }
}

void PidROS::set_gains(const Pid::Gains & gains)
{
  if (gains.i_min_ > gains.i_max_)
  {
    RCLCPP_ERROR(node_logging_->get_logger(), "received i_min > i_max, skip new gains");
  }
  else if (gains.u_min_ > gains.u_max_)
  {
    RCLCPP_ERROR(node_logging_->get_logger(), "received u_min > u_max, skip new gains");
  }
  else
  {
    pid_.set_gains(gains);
  }
}

void PidROS::publish_pid_state(double cmd, double error, rclcpp::Duration dt)
{
  Pid::Gains gains = pid_.get_gains();

  double p_error, i_term, d_error;
  get_current_pid_errors(p_error, i_term, d_error);

  // Publish controller state if configured
  if (rt_state_pub_)
  {
    pid_state_msg_.header.stamp = rclcpp::Clock().now();
    pid_state_msg_.timestep = dt;
    pid_state_msg_.error = error;
    pid_state_msg_.error_dot = d_error;
    pid_state_msg_.i_term = i_term;
    pid_state_msg_.p_gain = gains.p_gain_;
    pid_state_msg_.i_gain = gains.i_gain_;
    pid_state_msg_.d_gain = gains.d_gain_;
    pid_state_msg_.output = cmd;
    rt_state_pub_->tryPublish(pid_state_msg_);
  }
}

void PidROS::set_current_cmd(double cmd) { pid_.set_current_cmd(cmd); }

double PidROS::get_current_cmd() { return pid_.get_current_cmd(); }

void PidROS::get_current_pid_errors(double & pe, double & ie, double & de)
{
  double _pe, _ie, _de;
  pid_.get_current_pid_errors(_pe, _ie, _de);
  pe = _pe;
  ie = _ie;
  de = _de;
}

void PidROS::print_values()
{
  Pid::Gains gains = pid_.get_gains();

  double p_error, i_term, d_error;
  get_current_pid_errors(p_error, i_term, d_error);

  RCLCPP_INFO_STREAM(node_logging_->get_logger(),
                     "Current Values of PID template:\n"
                       << "  P Gain:       " << gains.p_gain_ << "\n"
                       << "  I Gain:       " << gains.i_gain_ << "\n"
                       << "  D Gain:       " << gains.d_gain_ << "\n"
                       << "  I Max:        " << gains.i_max_ << "\n"
                       << "  I Min:        " << gains.i_min_ << "\n"
                       << "  U_Max:                  " << gains.u_max_ << "\n"
                       << "  U_Min:                  " << gains.u_min_ << "\n"
                       << "  Tracking_Time_Constant: " << gains.trk_tc_ << "\n"
                       << "  Antiwindup:             " << gains.antiwindup_ << "\n"
                       << "  Antiwindup_Strategy:    " << gains.antiwindup_strat_.to_string()
                       << "\n"
                       << "\n"
                       << "  P Error:      " << p_error << "\n"
                       << "  I Term:       " << i_term << "\n"
                       << "  D Error:      " << d_error << "\n"
                       << "  Command:      " << get_current_cmd(););
}

void PidROS::set_parameter_event_callback()
{
  auto on_parameter_event_callback = [this](const std::vector<rclcpp::Parameter> & parameters)
  {
    rcl_interfaces::msg::SetParametersResult result;
    result.successful = true;

    /// @note don't use getGains, it's rt
    Pid::Gains gains = pid_.get_gains();
    bool changed = false;
    // The saturation parameter is special, it can change the u_min and u_max parameters
    // so we need to check it first and then proceed with the loop, as if we update only one
    // parameter, we need to keep this logic up-to-date. So, do not move it inside the loop
    bool saturation = true;  // default value
    try
    {
      // we can't use get_parameter_or, because we don't have access to a rclcpp::Node
      if (node_params_->has_parameter(param_prefix_ + "saturation"))
      {
        saturation = node_params_->get_parameter(param_prefix_ + "saturation").get_value<bool>();
      }
    }
    catch (const std::exception & e)
    {
      RCLCPP_ERROR_STREAM(
        node_logging_->get_logger(), "Error with saturation parameter: " << e.what());
    }

    std::for_each(
      parameters.begin(), parameters.end(),
      [&, this](const rclcpp::Parameter & parameter)
      {
        if (parameter.get_name() == param_prefix_ + "saturation")
        {
          saturation = parameter.get_value<bool>();
          changed = true;
          if (!saturation)
          {
            RCLCPP_WARN(
              node_logging_->get_logger(),
              "Saturation is set to false, Changing the u_min and u_max to -inf and inf");
            gains.u_min_ = -UMAX_INFINITY;
            gains.u_max_ = UMAX_INFINITY;
          }
          else
          {
            RCLCPP_INFO(
              node_logging_->get_logger(),
              "Saturation is set to true, using u_min and u_max from parameters");
            gains.u_min_ =
              node_params_->get_parameter(param_prefix_ + "u_clamp_min").get_value<double>();
            gains.u_max_ =
              node_params_->get_parameter(param_prefix_ + "u_clamp_max").get_value<double>();
          }
          return;
        }
      });

    for (auto & parameter : parameters)
    {
      const std::string param_name = parameter.get_name();
      try
      {
        if (param_name == param_prefix_ + "p")
        {
          gains.p_gain_ = parameter.get_value<double>();
          changed = true;
        }
        else if (param_name == param_prefix_ + "i")
        {
          gains.i_gain_ = parameter.get_value<double>();
          changed = true;
        }
        else if (param_name == param_prefix_ + "d")
        {
          gains.d_gain_ = parameter.get_value<double>();
          changed = true;
        }
        else if (param_name == param_prefix_ + "i_clamp_max")
        {
          gains.i_max_ = parameter.get_value<double>();
          changed = true;
        }
        else if (param_name == param_prefix_ + "i_clamp_min")
        {
          gains.i_min_ = parameter.get_value<double>();
          changed = true;
        }
        else if (param_name == param_prefix_ + "u_clamp_max")
        {
          gains.u_max_ = saturation ? parameter.get_value<double>() : UMAX_INFINITY;
          RCLCPP_WARN_EXPRESSION(
            node_logging_->get_logger(), !saturation,
            "Saturation is set to false, Changing the u_clamp_max inf");
          changed = true;
        }
        else if (param_name == param_prefix_ + "u_clamp_min")
        {
          gains.u_min_ = saturation ? parameter.get_value<double>() : -UMAX_INFINITY;
          RCLCPP_WARN_EXPRESSION(
            node_logging_->get_logger(), !saturation,
            "Saturation is set to false, Changing the u_clamp_min -inf");
          changed = true;
        }
        else if (param_name == param_prefix_ + "tracking_time_constant")
        {
          gains.trk_tc_ = parameter.get_value<double>();
          changed = true;
        }
        else if (param_name == param_prefix_ + "antiwindup")
        {
          gains.antiwindup_ = parameter.get_value<bool>();
          changed = true;
        }
        else if (param_name == param_prefix_ + "antiwindup_strategy")
        {
          gains.antiwindup_strat_ = AntiwindupStrategy(parameter.get_value<std::string>());
          changed = true;
        }
      }
      catch (const rclcpp::exceptions::InvalidParameterTypeException & e)
      {
        RCLCPP_INFO_STREAM(node_logging_->get_logger(), "Please use the right type: " << e.what());
      }
    }

    if (changed)
    {
      if (gains.i_min_ > gains.i_max_)
      {
        RCLCPP_ERROR(node_logging_->get_logger(), "Received i_min > i_max, skip new gains");
      }
      else if (gains.u_min_ > gains.u_max_)
      {
        RCLCPP_ERROR(node_logging_->get_logger(), "Received u_min > u_max, skip new gains");
      }
      else
      {
        pid_.set_gains(gains);
      }
    }

    return result;
  };
  /// @note this gets called whenever a parameter changes.
  /// Any parameter under that node. Not just PidROS.
  parameter_callback_ = node_params_->add_on_set_parameters_callback(on_parameter_event_callback);
}

}  // namespace control_toolbox
