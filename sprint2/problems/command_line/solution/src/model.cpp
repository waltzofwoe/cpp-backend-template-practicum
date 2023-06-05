#include "model.h"

#include <stdexcept>
#include <algorithm>
#include <ranges>

namespace rs = std::ranges;
namespace rv = std::ranges::views;

namespace model {
using namespace std::literals;

void Map::AddOffice(Office&& office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

void Game::AddMap(Map&& map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

GameSession* Game::FindSessionByMapId(const Map::Id& mapId){
    auto session = rs::find_if(_sessions, [mapId](auto arg){return arg.GetMapId() == mapId;});

    return session == _sessions.end() ? nullptr : &(*session);
}

GameSession& Game::CreateSession(const Map::Id& mapId){
    int sessionId = _sessions.size() + 1;

    return _sessions.emplace_back(GameSession{sessionId, mapId});
}

}  // namespace model
