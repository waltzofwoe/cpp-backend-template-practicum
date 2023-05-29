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

    class Application {
        std::vector<Player> _players;
        model::Game _game;

        public:
        Application(model::Game&& game) : _game { game } {};

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


        
    };
}