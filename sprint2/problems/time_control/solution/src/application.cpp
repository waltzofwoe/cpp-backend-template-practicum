#include <algorithm>
#include <ranges>
#include "application.h"
#include "token_generator.h"

namespace rs = std::ranges;

namespace app {

using namespace std::literals;

Player Application::JoinGame(const std::string& playerName, const std::string& mapId) {
    tokens::token_generator generator;

    if (auto session = _game.FindSessionByMapId(model::Map::Id{mapId})){
        auto findResult = rs::find_if(_players, [playerName, &session](auto arg){return arg.name == playerName && arg.sessionId == session->GetId();}); 
        
        // если игрока нет в сессии
        if (findResult == _players.end()) {
            Player player {_players.size() +1, playerName, session->GetId(), generator.create()};

            session->AddDog(player.id);

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

void Application::Move(const Player& player, std::string move){
    auto session = _game.GetSession(player.sessionId);

    if (!session)
        return;

    auto dog = session->GetDogByPlayerId(player.id);

    if (!dog)
        return;

    auto map = _game.FindMap(session->GetMapId());

    if (!map)
        return;

    auto speed = map->GetDogSpeed().has_value() 
        ? map->GetDogSpeed().value() 
        : _game.GetDefaultDogSpeed();

    if (move.empty())
    {
        dog->speedX = 0;
        dog->speedY = 0;

        return;
    }

    if (move == "L"s){
        dog->speedX = -1 * speed;
        dog->speedY = 0;

        return;
    }

    if (move == "R"s){
        dog->speedX = speed;
        dog->speedY = 0;

        return;
    }

    if (move == "U"s){
        dog->speedX = 0;
        dog->speedY = -1 * speed;

        return;
    }

    if (move == "D"s) {
        dog->speedX = 0;
        dog->speedY = speed;

        return;
    }
}
}