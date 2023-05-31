#include "api_handler.h"
#include "dto.h"
#include "json_loader.h"
#include <boost/json.hpp>
#include <ranges>
#include <set>

namespace http_handler {

namespace rs = std::ranges;
namespace rv = std::ranges::views;
namespace sys = boost::system;

    std::optional<std::string> GetAuthToken(StringRequest& request){
        std::string authorization = request[http::field::authorization];

        if (authorization.empty() || !std::regex_match(authorization, std::regex{"^Bearer \\w{32}"s})){
            return std::nullopt;
        }

        return authorization.substr(7);
    }


    JsonResponse HandleGetMaps(const app::Application& application, StringRequest&& request){
        // получить список карт
        auto maps = application.GetMaps();

        // спроецировать в DTO

        auto mapsDto = maps | rv::transform([](auto arg){return dto::MapRegistryDto {arg};});

        // отправить массив DTO
        return Json(request, mapsDto);
    }

    JsonResponse HandleGetMapByName(app::Application& application, StringRequest&& request, const std::string& mapName){

        auto map = application.FindMap(mapName);

        if (map == nullptr){
            return Json(request, dto::ErrorDto {"mapNotFound"s, "Map not found"s}, http::status::not_found);
        }

        return Json(request, map);
    }

    JsonResponse HandleJoinGame(app::Application& application, StringRequest&& request){
        if (request.method() != http::verb::post){
            auto response = Json(request, dto::ErrorDto {"invalidMethod"s, "Only POST method is expected"s}, http::status::method_not_allowed);
            response.set(http::field::allow, "POST");
            return response;
        }

        if (request[http::field::content_type] != "application/json"){
            return Json(request,
                dto::ErrorDto {"invalidContentType"s, "Expected application/json"s},
                http::status::bad_request);
        }

        sys::error_code ec;

        json::value body_value = json::parse(request.body(), ec);

        if (ec || !body_value.is_object())
        {
            return Json(request,
                dto::ErrorDto {"invalidArgument"s, ec.message()},
                http::status::bad_request);
        }

        auto body = body_value.as_object();

        if (!body.if_contains("mapId"s) || !body.if_contains("userName"s))
        {
            return Json(request,
                dto::ErrorDto {"invalidArgument"s, "Join game request parse error"s},
                http::status::bad_request);
        }

        std::string userName = json::value_to<std::string>(body["userName"s]);

        if (userName.empty())
        {
            return Json(request, 
                dto::ErrorDto {"invalidArgument"s, "Invalid name"s }, 
                http::status::bad_request);
        }

        auto mapId = json::value_to<std::string>(body["mapId"s]);

        auto map = application.FindMap(mapId);

        if (!map)
        {
            return Json(request,
                dto::ErrorDto {"mapNotFound"s, "Map not found"s},
                http::status::not_found);
        }

        auto player = application.JoinGame(userName, mapId);

        return Json(request, dto::AuthTokenDto {player.token, player.id});
    }

    JsonResponse HandleGetPlayers(app::Application& application, StringRequest&& request){
        if (request.method() != http::verb::get && request.method() != http::verb::head){
            auto response = Json(request, dto::ErrorDto {"invalidMethod"s, "Invalid method"s}, http::status::method_not_allowed);
            response.set(http::field::allow, "GET, HEAD"s);
            return response;
        }

        auto token = GetAuthToken(request);
        
        if (!token.has_value()){
            return Json(request, dto::ErrorDto {"invalidToken"s, "Authorization header is missing"s}, http::status::unauthorized);
        }

        auto player = application.FindPlayerByToken(*token);

        if (!player.has_value()){
            return Json(request, dto::ErrorDto{"unknownToken"s, "Player token has not been found"s}, http::status::unauthorized);
        }

        auto players = application.GetPlayersFromSession(player.value().sessionId);

        json::object jv;

        for(const auto &player : players){
            jv[std::to_string(player.id)] = { {"name"s, player.name} };
        }

        return Json(request, jv);
    }

    JsonResponse HandleGetGameState(app::Application& application, StringRequest&& request){
        if (request.method() != http::verb::get && request.method() != http::verb::head){
            auto response = Json(request, dto::ErrorDto {"invalidMethod"s, "Invalid method"s}, http::status::method_not_allowed);

            response.set(http::field::allow, "GET, HEAD"s);

            return response;
        }

        auto token = GetAuthToken(request);
        
        if (!token.has_value()){
            return Json(request, dto::ErrorDto {"invalidToken"s, "Authorization header is required"s}, http::status::unauthorized);
        }

        auto player = application.FindPlayerByToken(*token);

        if (!player.has_value()){
            return Json(request, dto::ErrorDto{"unknownToken"s, "Player token has not been found"s}, http::status::unauthorized);
        }

        auto session = application.GetSession(player.value().sessionId);

        if (!session){
            return Json(request, dto::ErrorDto { "sessionNotFound"s, "Session not found"s}, http::status::internal_server_error);
        }

        json::object obj;

        for (const auto dog : session->GetDogs()){

            std::string direction;

            switch (dog.direction)
            {
            case model::NORTH:
                direction = "U"s;
                break;

            case model::SOUTH:
                direction = "D"s;
                break;

            case model::EAST:
                direction = "R"s;
                break;

            case model::WEST:
                direction = "L"s;
                break;
            }

            obj[std::to_string(dog.playerId)] = {
                {"pos"s, {dog.x, dog.y}},
                {"speed"s, {dog.speedX, dog.speedY}},
                {"dir"s, direction}
            };

        }
        return Json(request, json::object {{"players"s, obj}});
    }

    JsonResponse HandlePostPlayerAction(app::Application& application, StringRequest&& request){
        if (request.method() != http::verb::post){
            auto response = Json(request, dto::ErrorDto {"invalidMethod"s, "Invalid method"s}, http::status::method_not_allowed);

            response.set(http::field::allow, "POST"s);

            return response;
        }

        auto token = GetAuthToken(request);
        
        if (!token.has_value()){
            return Json(request, dto::ErrorDto {"invalidToken"s, "Authorization header is required"s}, http::status::unauthorized);
        }

        auto player = application.FindPlayerByToken(*token);

        if (!player){
            return Json(request, dto::ErrorDto{"unknownToken"s, "Player token has not been found"s}, http::status::unauthorized);
        }

        if (request[http::field::content_type] != "application/json"s){
            return Json(request, dto::ErrorDto {"invalidArgument"s, "Invalid content type"s}, http::status::bad_request);
        }

        sys::error_code ec;

        auto body = json::parse(request.body(), ec);
        
        if (ec || !body.as_object().if_contains("move"s)) {
            return Json(request, dto::ErrorDto{"invalidArgument"s, "Failed to parse action"s}, http::status::bad_request);
        }

        auto move = json::value_to<std::string>(body.at("move"s));

        std::set<std::string> validValues {"L"s, "R"s, "U"s, "D"s, ""s};

        if (!validValues.contains(move))
        {
            return Json(request, dto::ErrorDto{"invalidArgument"s, "Failed to parse action"s}, http::status::bad_request);
        }

        application.Move(*player, move);

        return Json(request, {});
    }

    JsonResponse HandleBadRequest(StringRequest&& request){
        return Json(request, dto::ErrorDto {"badRequest"s, "Bad request"s}, http::status::bad_request);
    }
}