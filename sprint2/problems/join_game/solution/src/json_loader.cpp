#include "json_loader.h"
#include <fstream>
#include <sstream>
#include <iostream>

using namespace std::literals;

namespace json = boost::json;

namespace json_loader {

/// @brief Загрузить файл целиком
/// @param filePath 
/// @return 
std::string LoadFile(const std::filesystem::path& filePath) {
    std::ifstream ifs{filePath};

    if (!ifs.is_open()) {
        throw std::runtime_error("Не удалось открыть файл с конфигурацией");
    }

    std::ostringstream strbuf;

    strbuf << ifs.rdbuf();

    return strbuf.str();
}

std::vector<model::Map> LoadMaps(const json::value json_config)
{
    auto json_maps = json_config.as_object().at("maps"s);

    auto maps = json::value_to<std::vector<model::Map>>(json_maps);

    return maps;
}

model::Game LoadGame(const std::filesystem::path& json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла
    model::Game game;

    auto json_str = LoadFile(json_path);

    auto json_config = json::parse(json_str);

    auto maps = LoadMaps(json_config);

    for (const auto& map : maps) {
        game.AddMap(map);
    }

    return game;
}

} // namespace json_loader

namespace model {
Road tag_invoke(json::value_to_tag<Road>, const json::value &jv) {
    auto road_object = jv.as_object();

    auto x0 = road_object.at("x0"s).to_number<int>();

    auto y0 = road_object.at("y0"s).to_number<int>();

    if (road_object.if_contains("x1"s)){
        auto x1 = road_object.at("x1"s).to_number<int>();

        return {Road::HORIZONTAL, Point{x0, y0}, x1};
    }
    else if (road_object.if_contains("y1"s)) {
        auto y1 = road_object.at("y1"s).to_number<int>();

        return {Road::VERTICAL, Point{x0, y0}, y1};
    }

    throw std::runtime_error("Неправильный формат описания дороги"s);
}

void tag_invoke(json::value_from_tag, json::value& jv, const Road& road){
    jv = {
        {"x0"s, road.GetStart().x},
        {"y0"s, road.GetStart().y},
        {road.IsHorizontal() ? "x1" : "y1", road.IsHorizontal() ? road.GetEnd().x : road.GetEnd().y}
    };
}

Building tag_invoke(json::value_to_tag<Building>, const json::value& jv){
    return model::Building{
        model::Rectangle{
            model::Point{jv.at("x"s).to_number<int>() ,jv.at("y"s).to_number<int>()}, 
            model::Size{jv.at("w"s).to_number<int>(), jv.at("h"s).to_number<int>()}
        }
    };
}

void tag_invoke(json::value_from_tag, json::value &jv, const Building &building){
    jv = {
        {"x"s, building.GetBounds().position.x},
        {"y"s, building.GetBounds().position.y},
        {"w"s, building.GetBounds().size.width},
        {"h"s, building.GetBounds().size.height}
    };
}

Office tag_invoke(json::value_to_tag<Office>, const json::value& jv){
    return {
        model::Office::Id(json::value_to<std::string>(jv.at("id"s))),
        model::Point{jv.at("x"s).to_number<int>(), jv.at("y"s).to_number<int>()},
        model::Offset{jv.at("offsetX"s).to_number<int>(), jv.at("offsetY"s).to_number<int>()}
    };
}

void tag_invoke(json::value_from_tag, json::value &jv, const Office &office){
    jv = {
        {"id"s, *office.GetId()},
        {"x"s, office.GetPosition().x},
        {"y"s, office.GetPosition().y},
        {"offsetX"s, office.GetOffset().dx},
        {"offsetY"s, office.GetOffset().dy}
    };
}

Map tag_invoke(json::value_to_tag<Map>, const json::value& jv){
    auto id = Map::Id(json::value_to<std::string>(jv.at("id"s)));

    auto name = json::value_to<std::string>(jv.at("name"s));

    model::Map map{id, name};

    auto roads = json::value_to<std::vector<Road>>(jv.at("roads"s));

    for (const auto& road : roads) {
        map.AddRoad(road);
    }

    auto buildings = json::value_to<std::vector<Building>>(jv.at("buildings"s));

    for(const auto& building : buildings) {
        map.AddBuilding(building);
    }

    auto offices = json::value_to<std::vector<Office>>(jv.at("offices"s));

    for (const auto& office : offices){
        map.AddOffice(office);
    }

    return map;
}

void tag_invoke(json::value_from_tag, json::value &jv, const Map* map){
    json::object obj = {
        {"id"s, *map->GetId()},
        {"name"s, map->GetName()}
    };

    obj["roads"s] = json::value_from(map->GetRoads());

    obj["buildings"] = json::value_from(map->GetBuildings());

    obj["offices"] = json::value_from(map->GetOffices());

    jv= obj;
}
} // namespace model

namespace dto {
    void tag_invoke(json::value_from_tag, json::value& jv, const MapRegistryDto& dto){
        jv = {
            {"id"s, dto.Id},
            {"name"s, dto.Name}
        };
    };

    void tag_invoke(json::value_from_tag, json::value& jv, const ErrorDto& dto){
        jv= {
            {"code"s, dto.Code},
            {"message"s, dto.Message}
        };
    };

    void tag_invoke(json::value_from_tag, json::value& jv, const AuthTokenDto& dto){
        jv = {
            {"authToken"s, dto.AuthToken},
            {"playerId"s, dto.PlayerId}
        };
    }
} //namespace dto