#pragma once

#include <filesystem>
#include <boost/json.hpp>

#include "model.h"
#include "dto.h"

namespace json = boost::json;

namespace json_loader {

model::Game LoadGame(const std::filesystem::path& json_path);

}  // namespace json_loader

namespace model {
    Road tag_invoke(json::value_to_tag<Road>, const json::value& jv);

    void tag_invoke(json::value_from_tag, json::value& jv, const Road& road);

    Building tag_invoke(json::value_to_tag<Building>, const json::value& jv);

    void tag_invoke(json::value_from_tag, json::value& jv, const Building& building);

    Office tag_invoke(json::value_to_tag<Office>, const json::value& jv);

    void tag_invoke(json::value_from_tag, json::value& jv, const Office& office);

    Map tag_invoke(json::value_to_tag<Map>, const json::value& jv);

    void tag_invoke(json::value_from_tag, json::value& jv, const Map* map);
}

namespace dto {
    void tag_invoke(json::value_from_tag, json::value& jv, const MapRegistryDto& dto);
    void tag_invoke(json::value_from_tag, json::value& jv, const ErrorDto& dto);
    void tag_invoke(json::value_from_tag, json::value& jv, const AuthTokenDto& dto);
}
