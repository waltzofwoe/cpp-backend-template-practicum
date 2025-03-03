#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <ranges>
#include <algorithm>

#include "tagged.h"

namespace rs = std::ranges;

namespace model {

using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

class Road {
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
    }

    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point GetStart() const noexcept {
        return start_;
    }

    Point GetEnd() const noexcept {
        return end_;
    }

private:
    Point start_;
    Point end_;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    std::optional<double> GetDogSpeed() const noexcept {
        return _dogSpeed;
    }

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);

    void SetDogSpeed(double speed) {
        _dogSpeed.emplace(speed);
    }

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;
    std::optional<double> _dogSpeed;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
};

enum Direction {
    NORTH,
    SOUTH,
    WEST,
    EAST
};

struct Dog {
    std::string name;
    int id;
    int playerId;
    double x;
    double y;
    double speedX;
    double speedY;
    Direction direction;

    public:
    Dog(int id, int playerId) : id { id }, playerId { playerId }, direction{NORTH}, speedX{}, speedY{} {};
};

class GameSession {
    int _id;
    Map::Id _mapId;
    std::vector<Dog> _dogs;

    public:
    explicit GameSession(int id, const Map::Id& mapId) : _id {id}, _mapId {mapId} {};

    const Map::Id& GetMapId() {
        return _mapId;
    }

    int GetId() {
        return _id;
    }

    void AddDog(int playerId) {
        int dogId = _dogs.size() + 1;

        _dogs.emplace_back(Dog {dogId, playerId});
    }

    const std::vector<Dog>& GetDogs(){
        return _dogs;
    }

    Dog* GetDogByPlayerId(int playerId) {
        auto dog = rs::find_if(_dogs, [playerId](auto arg){return arg.playerId == playerId;});

        return dog == _dogs.end()
            ?  nullptr
            : &(*dog);
    }
};

class Game {
public:
    using Maps = std::vector<Map>;

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    const Map* FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }

    GameSession* GetSession(int sessionId){
        auto result = rs::find_if(_sessions, [sessionId](auto arg){return arg.GetId() == sessionId;});

        return result == _sessions.end() ? nullptr : &(*result);
    }

    GameSession* FindSessionByMapId(const Map::Id& mapId);
    GameSession& CreateSession(const Map::Id& mapId);

    void SetDefaultDogSpeed(double speed) { _defaultDogSpeed = speed; }
    int GetDefaultDogSpeed() const {return _defaultDogSpeed; }

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;

    std::vector<GameSession> _sessions;
    double _defaultDogSpeed = 1;
};

}  // namespace model
