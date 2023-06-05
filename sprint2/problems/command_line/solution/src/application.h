#pragma once
#include <vector>
#include <string>
#include "model.h"

namespace app {
    
    struct Player {
        int id;
        std::string name;
        int sessionId;
        std::string token;
    };

    struct Collision {
        double x_min;
        double x_max;
        double y_min;
        double y_max;
    };

    class Application {
        std::vector<Player> _players;
        model::Game& _game;
        bool _randomizeSpawnPoints;

        std::vector<Collision> GetCollisionsAtPosition(model::Position& position, model::Map::Id mapId);
        model::Position GetSpawnPoint(const model::Map* map);

        public:
        explicit Application(model::Game& game, bool randomizeSpawnPoints) : _game { game }, _randomizeSpawnPoints{randomizeSpawnPoints} {};

        Player JoinGame(const std::string& playerName, const std::string& mapId);

        std::optional<Player> FindPlayerByToken(const std::string& token);

        std::vector<Player> GetPlayersFromSession(int sessionId);

        const model::Game::Maps GetMaps() const noexcept {
            return _game.GetMaps();
        }

        const model::Map* FindMap(const std::string& mapName){
            auto mapId = model::Map::Id {mapName};

            return _game.FindMap(mapId);
        }

        model::GameSession* GetSession(int sessionId){
            return _game.GetSession(sessionId);
        }

        void Move(const Player& player, std::string move);

        void AddTime(int64_t timeDelta);
    };
}