#pragma once

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/json.hpp>

namespace logger {

BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", boost::json::value)

void InitBoostLogs();

void Info(const std::string& message, boost::json::value custom_data);

void Error(const std::string& message, boost::json::value cusom_data);

void Fatal(const std::string& message, boost::json::value custom_data);

} //namespace logger