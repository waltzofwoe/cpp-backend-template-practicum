#include <algorithm>
#include <ranges>
#include <utility>
#include "application.h"
#include "token_generator.h"

namespace app {
namespace rs = std::ranges;
namespace rv = rs::views;

using namespace std::literals;

Player Application::JoinGame(const std::string& playerName, const std::string& mapId) {
    tokens::token_generator generator;

    auto map = _game.GetMaps().at(0);

    auto roadStart = map.GetRoads().at(0).GetStart();

    model::Position initPosition {roadStart.x, roadStart.y};

    if (auto session = _game.FindSessionByMapId(model::Map::Id{mapId})){
        auto findResult = rs::find_if(_players, [playerName, &session](auto arg){return arg.name == playerName && arg.sessionId == session->GetId();}); 
        
        // если игрок есть - возвращаем что есть
        if (findResult != _players.end()) {
            return *findResult;

        }

        // если игрока нет в сессии

        Player player {_players.size() +1, playerName, session->GetId(), generator.create()};

        session->AddDog(player.id, initPosition);

        return _players.emplace_back(player);
    }

    // сессии нет - создаем

    auto& session = _game.CreateSession(model::Map::Id{mapId});

    Player player {_players.size() +1, playerName, session.GetId(), generator.create()};

    session.AddDog(player.id, initPosition);

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
        dog->speed.vx = 0;
        dog->speed.vy = 0;
        dog->direction = model::NORTH;

        return;
    }

    if (move == "L"s){
        dog->speed.vx = -1 * speed;
        dog->speed.vy = 0;
        dog->direction = model::WEST;

        return;
    }

    if (move == "R"s){
        dog->speed.vx = speed;
        dog->speed.vy = 0;
        dog->direction = model::EAST;

        return;
    }

    if (move == "U"s){
        dog->speed.vx = 0;
        dog->speed.vy = -1 * speed;
        dog->direction = model::NORTH;

        return;
    }

    if (move == "D"s) {
        dog->speed.vx = 0;
        dog->speed.vy = speed;
        dog->direction = model::SOUTH;

        return;
    }
}

void Application::AddTime(int64_t timeDelta){
    auto& sessions = _game.GetSessions();

    for(auto& session : sessions) {
        auto& dogs = session.GetDogs();

        auto map = session.GetMapId();


        for (auto& dog : dogs){
            auto collisions = GetCollisionsAtPosition(dog.coord, map);

            // расчет новой координаты по оси x
            dog.coord.x += dog.speed.vx * (timeDelta / 1000.0);

            // проверка коллизий по x
            auto xmin = rs::min(collisions | rv::transform(&Collision::x_min));
            auto xmax = rs::max(collisions | rv::transform(&Collision::x_max));

            if (dog.coord.x < xmin || dog.coord.x > xmax){
                dog.coord.x = dog.coord.x < xmin ? xmin : xmax;
                dog.speed.vx = 0;
            }

            // расчет новой координаты по y

            dog.coord.y += dog.speed.vy * (timeDelta / 1000.0);

            auto ymin = rs::min(collisions | rv::transform(&Collision::y_min));
            auto ymax = rs::max(collisions | rv::transform(&Collision::y_max));

            if (dog.coord.y < ymin || dog.coord.y > ymax){
                dog.coord.y = dog.coord.y < ymin ? ymin : ymax;
                dog.speed.vy = 0;
            }
        }
    }
}

/// 
double GetD(model::Position a, model::Position b, model::Position p){
    return (b.x - a.x) * (p.y - a.y) - (p.x - a.x) * (b.y - a.y);
}

std::vector<Collision> Application::GetCollisionsAtPosition(model::Position& position, model::Map::Id mapId){
    auto roads = _game.FindMap(mapId)->GetRoads();

    const auto delta = 0.4;

    std::vector<Collision> result;

    for(const auto& road: roads) {
        auto x1 = std::min(road.GetStart().x, road.GetEnd().x) - delta;
        auto x2 = std::max(road.GetStart().x, road.GetEnd().x) + delta;
        auto y1 = std::min(road.GetStart().y, road.GetEnd().y) - delta;
        auto y2 = std::max(road.GetStart().y, road.GetEnd().y) + delta;

        std::vector<std::pair<model::Position, model::Position>> lines;

        // строим прямоугольник по часовой стрелке
        lines.emplace_back(std::pair{model::Position{x1, y1}, model::Position{x1, y2}});
        lines.emplace_back(std::pair{model::Position{x1, y2}, model::Position{x2, y2}});
        lines.emplace_back(std::pair{model::Position{x2, y2}, model::Position{x2, y1}});
        lines.emplace_back(std::pair{model::Position{x2, y1}, model::Position{x1, y1}});

        // собака внутри прямоугольника, если все точки справа от граней (поскольку построение идет по часовой стрелке)
        auto dogOnRoad = rs::all_of(lines, [&position](auto arg){ return GetD(arg.first, arg.second, position) <= 0;});

        if (dogOnRoad) {
            result.emplace_back(Collision {x1,x2,y1,y2});
        }
    }

    return result;
}
}