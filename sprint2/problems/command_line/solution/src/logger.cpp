#include "logger.h"

#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/date_time.hpp>
#include <boost/json.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>

namespace logs = boost::log;
namespace expr = boost::log::expressions;
namespace attrs = boost::log::attributes;
namespace json = boost::json;

using namespace std::literals;

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)


namespace logger{
void JsonFormatter(const logs::record_view& record, logs::formatting_ostream& strm) {
    auto ts = record[timestamp];

    std::string message = *record[expr::smessage];

    json::object jv = {
        {"timestamp", boost::posix_time::to_iso_extended_string(*ts)},
        {"message", message}
    };

    if (!record[additional_data].empty())
    {
        jv["data"] = *record[additional_data];
    }

    auto json_output = json::serialize(jv);

    strm << json_output << std::endl;
}


void InitBoostLogs(){
    logs::add_common_attributes();

    logs::add_console_log(
        std::cout,
        logs::keywords::auto_flush = true,
        logs::keywords::format = &JsonFormatter
    );
};

void Info(const std::string& message, boost::json::value custom_data){
    BOOST_LOG_TRIVIAL(info) << logs::add_value(logger::additional_data, custom_data)
                            << message;
}

void Error(const std::string& message, boost::json::value custom_data){
    BOOST_LOG_TRIVIAL(error) << logs::add_value(logger::additional_data, custom_data)
                             << message;
}

void Fatal(const std::string& message, boost::json::value custom_data){
    BOOST_LOG_TRIVIAL(fatal) << logs::add_value(logger::additional_data, custom_data)
                             << message;
}

}