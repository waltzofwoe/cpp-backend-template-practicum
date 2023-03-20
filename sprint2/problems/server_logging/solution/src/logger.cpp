#include "logger.h"

#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/date_time.hpp>
#include <boost/json.hpp>

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

    json::value jv = {
        {"timestamp", boost::posix_time::to_iso_extended_string(*ts)},
        {"message", message}
    };

    auto json_output = json::serialize(jv);

    strm << json_output << std::endl;
}


void InitBoostLogs(){
    logs::add_common_attributes();

    logs::add_console_log(
        std::cout,
        logs::keywords::format = &JsonFormatter
    );
};
}