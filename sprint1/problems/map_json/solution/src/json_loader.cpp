#include "json_loader.h"
#include <fstream>
#include <sstream>
#include <boost/json.hpp>

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
}

model::Game LoadGame(const std::filesystem::path& json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла
    model::Game game;

    auto json_str = LoadFile(json_path);

    auto json_obj = json::parse(json_str);
    
    auto json_maps = json_obj.as_object().at("maps"s).as_array();

    for (const auto& json_map : json_maps) {
        auto id = model::Map::Id(json::value_to<std::string>(json_map.at("id"s)));

        auto name = json::value_to<std::string>(json_map.at("name"s));

        model::Map map{id, name};

        auto json_roads = json_map.at("roads"s).as_array();

        for (const auto& json_road : json_roads) {
            auto x0 = json_road.at("x0"s).to_number<int>();

            auto y0 = json_road.at("y0"s).to_number<int>();

            if (json_road.at("x1"s).is_number()){
                auto x1 = json_road.at("x1"s).to_number<int>();

                map.AddRoad({model::Road::HORIZONTAL, model::Point{x0, y0}, x1});
            }

            if (json_road.at("y1"s).is_number()){
                auto y1 = json_road.at("y1"s).to_number<int>();

                map.AddRoad({model::Road::VERTICAL, model::Point{x0, y0}, y1});
            }
        }

        auto json_buildings = json_map.at("buildings"s).as_array();

        for(const auto& json_building : json_buildings) {
            map.AddBuilding(model::Building
                {model::Rectangle{
                    model::Point{json_building.at("x"s).to_number<int>() ,json_building.at("y"s).to_number<int>()}, 
                    model::Size{json_building.at("w"s).to_number<int>(), json_building.at("h"s).to_number<int>()}
                }
            });
        }

        auto json_offices = json_map.at("offices"s).as_array();

        for (const auto& json_office : json_offices){
            map.AddOffice({
                model::Office::Id(json::value_to<std::string>(json_office.at("id"s))),
                model::Point{json_office.at("x"s).to_number<int>(), json_office.at("y"s).to_number<int>()},
                model::Offset{json_office.at("offsetX"s).to_number<int>(), json_office.at("offsetY"s).to_number<int>()}
                });
        }

        game.AddMap(map);
    }

    return game;
}

}  // namespace json_loader
