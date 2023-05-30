#include <algorithm>
#include <ranges>
#include "application.h"
#include "token_generator.h"

namespace rs = std::ranges;

namespace app {

Player Application::JoinGame(const std::string& playerName, const std::string& mapId) {
    tokens::token_generator generator;

    if (auto session = _game.FindSessionByMapId(model::Map::Id{mapId}); session.has_value()){
        auto findResult = rs::find_if(_players, [playerName, &session](auto arg){return arg.name == playerName && arg.sessionId == session.value().GetId();}); 
        
        // если игрока нет в сессии
        if (findResult == _players.end()) {
            Player player {_players.size() +1, playerName, session.value().GetId(), generator.create()};

            session.value().AddDog(player.id);

            return _players.emplace_back(player);
        }

        // если игрок есть - возвращаем что есть
        return *findResult;
    }

    // сессии нет - создаем

    auto& session = _game.CreateSession(model::Map::Id{mapId});

    Player player {_players.size() +1, playerName, session.GetId(), generator.create()};

    session.AddDog(player.id);

    return _players.emplace_back(player);
}

std::vector<Player> Application::GetPlayersFromSession(int sessionId) {
    auto result = _players | rs::views::filter([sessionId](auto arg){return arg.sessionId == sessionId;});

    return std::vector(result.begin(), result.end());
}

std::optional<Player> Application::FindPlayerByToken(const std::string& token){
    auto player = rs::find_if(_players, [&token](auto arg) {return arg.token == token;});

    if (player == _players.end())
    {
        return std::nullopt;
    }

    return *player;
}

}