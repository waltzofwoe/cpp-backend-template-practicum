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

PlayerToken Game::GetPlayerToken(std::string_view playerName){
    auto existingPlayer = rs::find_if(_players, [&playerName](auto arg){return arg.name == playerName;});

    tokens::token_generator token_generator;

    if (existingPlayer != _players.end()){
        auto existingToken = rs::find_if(_tokens, [&existingPlayer](auto arg){return arg.playerId == (*existingPlayer).id;});

        if (existingToken != _tokens.end()){
            return *existingToken;
        }

        model::PlayerToken token {token_generator.create(), (*existingPlayer).id};

        _tokens.emplace_back(token);

        return token;
    }

    int playerId = _players.size() + 1;

    model::Player newPlayer {playerId, std::string {playerName}};

    _players.emplace_back(newPlayer);

    model::PlayerToken newToken { token_generator.create(), playerId };

    _tokens.emplace_back(newToken);

    return newToken;
}
}  // namespace model
