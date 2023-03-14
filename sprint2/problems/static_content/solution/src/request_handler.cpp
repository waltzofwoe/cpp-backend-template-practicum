#include "request_handler.h"
#include "json_loader.h"
#include <boost/json.hpp>

namespace json = boost::json;
using namespace std::literals;


namespace http_handler {

std::string RequestHandler::GetMapsJson(){
    auto maps = game_.GetMaps();

    json::array json_array;

    for(const auto& map : maps){
        json::object jv;

        jv["id"s] = *map.GetId();
        jv["name"s] = map.GetName();

        json_array.emplace_back(jv);
    }

    auto result = json::serialize(json_array);

    return result;
}

std::string RequestHandler::GetMapJson(const model::Map* map){

    auto obj = json::value_from(map);

    return json::serialize(obj);
}

RequestHandler::JsonResponse RequestHandler::MakeJsonResponse(http::status status, unsigned http_version, std::string_view body){
    JsonResponse response(status, http_version);
    response.body() = body;
    response.content_length(response.body().size());
    response.set(http::field::content_type, "application/json");

    return response;
}

RequestHandler::JsonResponse RequestHandler::MakeErrorResponse(
    http::status status, 
    unsigned http_version, 
    std::string_view code, 
    std::string_view message){
    json::value jv({{"code", code}, {"message", message}});

    JsonResponse response(status, http_version);
    response.body() = json::serialize(jv);
    response.content_length(response.body().size());
    response.set(http::field::content_type, "application/json");

    return response;
}

}  // namespace http_handler
