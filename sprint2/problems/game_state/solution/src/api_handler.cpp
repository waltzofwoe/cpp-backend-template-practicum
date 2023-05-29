#include "api_handler.h"
#include "dto.h"
#include "json_loader.h"
#include <boost/json.hpp>
#include <ranges>

namespace rs = std::ranges;
namespace rv = std::ranges::views;
namespace sys = boost::system;

namespace http_handler {
    JsonResponse HandleGetMaps(const app::Application& application, StringRequest request){
        // получить список карт
        auto maps = application.GetMaps();

        // спроецировать в DTO

        auto mapsDto = maps | rv::transform([](auto arg){return dto::MapRegistryDto {arg};});

        // отправить массив DTO
        return Json(request, mapsDto);
    }

    JsonResponse HandleGetMapByName(app::Application& application, StringRequest request, const std::string& mapName){

        auto map = application.FindMap(mapName);

        if (map == nullptr){
            return Json(request, dto::ErrorDto {"mapNotFound"s, "Map not found"s}, http::status::not_found);
        }

        return Json(request, map);
    }

    JsonResponse HandleJoinGame(app::Application& application, StringRequest request){
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

    JsonResponse HandleGetPlayers(app::Application& application, StringRequest request){
        if (request.method() != http::verb::get && request.method() != http::verb::head){
            auto response = Json(request, dto::ErrorDto {"invalidMethod"s, "Invalid method"s}, http::status::method_not_allowed);
            response.set(http::field::allow, "GET, HEAD"s);
            return response;
        }

        std::string authorization = request[http::field::authorization];

        if (authorization.empty() || !std::regex_match(authorization, std::regex{"^Bearer .+"s})){
            return Json(request, dto::ErrorDto {"invalidToken"s, "Authorization header is missing"s}, http::status::unauthorized);
        }

        auto token = authorization.substr(7);

        auto player = application.FindPlayerByToken(token);

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

    JsonResponse HandleBadRequest(StringRequest request){
        return Json(request, dto::ErrorDto {"badRequest"s, "Bad request"s}, http::status::bad_request);
    }
}