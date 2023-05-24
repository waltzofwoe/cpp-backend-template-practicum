#include "model.h"

#include <stdexcept>
#include <algorithm>
#include "token_generator.h"

namespace rs = std::ranges;
namespace rv = std::ranges::views;

namespace model {
using namespace std::literals;

void Map::AddOffice(Office office) {
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

void Game::AddMap(Map map) {
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

Player Game::GetOrCreatePlayer(std::string_view playerName){
    auto existingPlayer = rs::find_if(_players, [&playerName](auto arg){return arg.name == playerName;});

    if (existingPlayer != _players.end()){
        return *existingPlayer;
    }

    tokens::token_generator token_generator;

    int playerId = _players.size() + 1;

    model::Player newPlayer {playerId, std::string {playerName}, token_generator.create()};

    _players.emplace_back(newPlayer);

    return newPlayer;
}

std::optional<Player> Game::FindPlayerByToken(std::string_view token){
    auto player = rs::find_if(_players, [&token](auto arg) {return arg.token == token;});

    if (player == _players.end())
    {
        return std::nullopt;
    }

    return *player;
}
}  // namespace model
