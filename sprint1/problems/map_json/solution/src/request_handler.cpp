#include "request_handler.h"
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
    json::object obj{
        {"id"s, *map->GetId()},
        {"name"s, map->GetName()}
    };

    obj["roads"].emplace_array();

    for(const auto& road : map->GetRoads()) {
        json::object road_json {
            {"x0"s, road.GetStart().x},
            {"y0"s, road.GetStart().y},
            {road.IsHorizontal() ? "x1" : "y1", road.IsHorizontal() ? road.GetEnd().x : road.GetEnd().y}
        };

        obj["roads"].as_array().emplace_back(road_json);
    }

    obj["buildings"].emplace_array();

    for(const auto& building : map->GetBuildings()) {
        json::object building_json {
            {"x"s, building.GetBounds().position.x},
            {"y"s, building.GetBounds().position.y},
            {"w"s, building.GetBounds().size.width},
            {"h"s, building.GetBounds().size.height}
        };

        obj["buildings"].as_array().emplace_back(building_json);
    }

    obj["offices"].emplace_array();

    for(const auto& office : map->GetOffices()){
        json::object office_json {
            {"id"s, *office.GetId()},
            {"x"s, office.GetPosition().x},
            {"y"s, office.GetPosition().y},
            {"offsetX"s, office.GetOffset().dx},
            {"offsetY"s, office.GetOffset().dy}
        };

        obj["offices"].as_array().emplace_back(office_json);
    }

    return json::serialize(obj);
}

RequestHandler::JsonResponse RequestHandler::MakeJsonResponse(http::status status, unsigned http_version, std::string_view body){
    JsonResponse response(status, http_version);
    response.body() = body;
    response.content_length(response.body().size());
    response.set(http::field::content_type, "application/json");

    return response;
}

RequestHandler::JsonResponse RequestHandler::MakeErrorResponse(http::status status, unsigned http_version, std::string_view code, std::string_view message){
    json::value jv({{"code", code}, {"message", message}});

    JsonResponse response(status, http_version);
    response.body() = json::serialize(jv);
    response.content_length(response.body().size());
    response.set(http::field::content_type, "application/json");

    return response;
}


}  // namespace http_handler
