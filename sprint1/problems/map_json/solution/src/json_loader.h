#pragma once

#include <filesystem>
#include <boost/json.hpp>
#include <vector>

#include "model.h"

namespace json = boost::json;

namespace json_loader {

model::Game LoadGame(const std::filesystem::path& json_path);

std::vector<model::Map> LoadMaps(const json::value json_config);

}  // namespace json_loader

namespace model {
    Road tag_invoke(json::value_to_tag<Road>, const json::value& jv);

    Building tag_invoke(json::value_to_tag<Building>, const json::value& jv);

    Office tag_invoke(json::value_to_tag<Office>, const json::value& jv);

    Map tag_invoke(json::value_to_tag<Map>, const json::value& jv);
}