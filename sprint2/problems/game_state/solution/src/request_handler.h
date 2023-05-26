#pragma once
#include <filesystem>
#include <iostream>
#include <memory>

#include "http_server.h"
#include "model.h"
#include "dto.h"
#include "file_utils.h"
#include "logger.h"
#include <regex>

#define BOOST_URL_NO_LIB
#include <boost/url.hpp>
#include <boost/json.hpp>

#define BOOST_BEAST_USE_STD_STRING_VIEW

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace url = boost::urls;
namespace fs = std::filesystem;
namespace sys = boost::system;
namespace json = boost::json;
namespace logs = boost::log;

using StringResponse = http::response<http::string_body>;
using EmptyResponse = http::response<http::empty_body>;
using FileResponse = http::response<http::file_body>;

using namespace std::literals;

std::string_view mime_type(fs::path path);

template <typename Body, typename Allocator>
StringResponse BadRequest(const http::request<Body, http::basic_fields<Allocator>>& request, const std::string& message) {
    StringResponse response { http::status::bad_request, request.version()};
    response.set(http::field::content_type, "text/plain");
    response.keep_alive(request.keep_alive());
    response.body() = message;
    response.prepare_payload();
    return response;
};

template <typename Body, typename Allocator>
StringResponse NotFound(const http::request<Body, http::basic_fields<Allocator>>& request, const std::string& message) {
    StringResponse response { http::status::not_found, request.version()};
    response.set(http::field::content_type, "text/plain");
    response.keep_alive(request.keep_alive());
    response.body() = message;
    response.prepare_payload();
    return response;
};

template <typename Body, typename Allocator>
StringResponse InternalError(const http::request<Body, http::basic_fields<Allocator>>& request, const std::string& message) {
    StringResponse response { http::status::internal_server_error, request.version()};
    response.set(http::field::content_type, "text/plain");
    response.keep_alive(request.keep_alive());
    response.body() = message;
    response.prepare_payload();
    return response;
};

template <typename Body, typename Allocator, typename Object>
StringResponse Json(
    const http::request<Body, http::basic_fields<Allocator>>& request, 
    Object object,
    http::status status_code = http::status::ok){
    auto jv = json::value_from(object);

    auto json_string = json::serialize(jv);

    StringResponse response {status_code, request.version()};
    response.set(http::field::content_type, "application/json");
    response.set(http::field::cache_control, "no-cache");
    response.body() = json_string;
    response.keep_alive(request.keep_alive());
    response.prepare_payload();
    return response;
}

template <typename Body, typename Allocator>
StringResponse Json(
    const http::request<Body, http::basic_fields<Allocator>>& request, 
    json::value jvalue,
    http::status status_code = http::status::ok)
{
    auto json_string = json::serialize(jvalue);
    StringResponse response {status_code, request.version()};
    response.set(http::field::content_type, "application/json");
    response.body() = json_string;
    response.keep_alive(request.keep_alive());
    response.prepare_payload();
    return response;
}

class StaticFileRequestHandler {
public:
    StaticFileRequestHandler(fs::path wwwroot);

    StaticFileRequestHandler(const StaticFileRequestHandler&) =delete;
    StaticFileRequestHandler& operator=(const StaticFileRequestHandler&) = delete;

    StaticFileRequestHandler(StaticFileRequestHandler&& other):
        wwwroot_{other.wwwroot_} {}

    template <typename Body, typename Allocator, typename ResponseWriter>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, ResponseWriter&& writer) {
        if( req.method() != http::verb::get &&
            req.method() != http::verb::head){
            writer(std::move(BadRequest(req, "Method not allowed"s)));
            return;
        }

        if(req.target().empty() ||
            req.target()[0] != '/'){
            writer(std::move(BadRequest(req, "Bad request"s)));
            return;
        }

        url::decode_view decoded_target(req.target());

        std::string decoded_path {decoded_target.begin(), decoded_target.end()};

        if (decoded_path.back() == '/')
        {
            decoded_path = decoded_path.append("index.html"s);
        }

        auto file_path = fs::weakly_canonical(wwwroot_ / decoded_path.substr(1));

        if (file_path.generic_string().back() == '/') {
            file_path = file_path / "index.html"s;
        }

        if (!IsSubPath(file_path, wwwroot_)){
            writer(std::move(BadRequest(req, "Incorrent path"s)));
            return;
        }

        http::file_body::value_type file_body;

        sys::error_code ec;

        file_body.open(file_path.c_str(), beast::file_mode::scan, ec);

        if (ec == sys::errc::no_such_file_or_directory){
            writer(std::move(NotFound(req, "File not found"s)));
            return;
        }

        if (ec) {
            writer(std::move(InternalError(req, ec.message())));
            return;
        }

        auto const size = file_body.size();

        if (req.method() == http::verb::head){
            EmptyResponse response {http::status::ok, req.version()};
            response.set(http::field::content_type, mime_type(file_path));
            response.content_length(size);
            response.keep_alive(req.keep_alive());
            
            writer(std::move(response));
            return;
        }

        FileResponse response { 
            std::piecewise_construct,
            std::make_tuple(std::move(file_body)),
            std::make_tuple(http::status::ok, req.version())};

        response.set(http::field::content_type, mime_type(file_path));
        response.content_length(size);
        response.keep_alive(req.keep_alive());
        writer(std::move(response));
    }
private:
    fs::path wwwroot_;
};

class JoinRequestHandler{
    model::Game& _game;

public:
    explicit JoinRequestHandler(model::Game& game) : _game { game }{};

    template <typename Body, typename Allocator, typename ResponseWriter>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& request, ResponseWriter&& writer) {
        if (request.method() != http::verb::post){
            auto response = Json(request, dto::ErrorDto {"invalidMethod"s, "Only POST method is expected"s}, http::status::method_not_allowed);
            response.set(http::field::allow, "POST");
            writer(response);
            return;
        }

        if (request[http::field::content_type] != "application/json"){
            writer(Json(request,
                dto::ErrorDto {"invalidContentType"s, "Expected application/json"s},
                http::status::bad_request));

            return;
        }

        sys::error_code ec;

        json::value body_value = json::parse(request.body(), ec);

        if (ec || !body_value.is_object())
        {
            writer(Json(request,
                dto::ErrorDto {"invalidArgument"s, ec.message()},
                http::status::bad_request));
            
            return;
        }

        auto body = body_value.as_object();

        if (!body.if_contains("mapId"s) || !body.if_contains("userName"s))
        {
            writer(Json(request,
                dto::ErrorDto {"invalidArgument"s, "Join game request parse error"s},
                http::status::bad_request));
            
            return;
        }

        std::string userName = json::value_to<std::string>(body["userName"s]);

        if (userName.empty())
        {
            writer(Json(request, 
                dto::ErrorDto {"invalidArgument"s, "Invalid name"s }, 
                http::status::bad_request));
            return;
        }

        auto mapId = json::value_to<std::string>(body["mapId"s]);

        auto map = _game.FindMap(model::Map::Id {mapId});

        if (!map)
        {
            writer(Json(request,
                dto::ErrorDto {"mapNotFound"s, "Map not found"s},
                http::status::not_found));

            return;
        }

        auto player = _game.GetOrCreatePlayer(userName);

        writer(Json(request, dto::AuthTokenDto {player.token, player.id}));

        return;
    }
};

class GetPlayersRequestHandler {
    model::Game& _game;

    public:
    explicit GetPlayersRequestHandler(model::Game& game) : _game {game} {};

    template <typename Body, typename Allocator, typename ResponseWriter>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& request, ResponseWriter&& writer) {
        if (request.method() != http::verb::get && request.method() != http::verb::head){
            auto response = Json(request, dto::ErrorDto {"invalidMethod"s, "Invalid method"s}, http::status::method_not_allowed);
            response.set(http::field::allow, "GET, HEAD"s);
            writer(response);
            return;
        }

        std::string authorization = request[http::field::authorization];

        if (authorization.empty() || !std::regex_match(authorization, std::regex{"^Bearer .+"s})){
            auto response = Json(request, dto::ErrorDto {"invalidToken"s, "Authorization header is missing"s}, http::status::unauthorized);
            writer(response);
            return;
        }

        auto token = authorization.substr(7);

        auto player = _game.FindPlayerByToken(token);

        if (!player.has_value()){
            auto response = Json(request, dto::ErrorDto{"unknownToken"s, "Player token has not been found"s}, http::status::unauthorized);
            writer(response);
            return;
        }

        auto players = _game.GetPlayers();

        json::object jv;

        for(const auto &player : players){
            jv[std::to_string(player.id)] = { {"name"s, player.name} };
        }

        auto response = Json(request, jv);

        writer(response);
    }
};

class RequestHandler {
public:

    using JsonResponse = http::response<http::string_body>;
    using StatusCodeResponse = http::response<http::empty_body>;
    using FileResponse = http::response<http::file_body>;

    explicit RequestHandler(model::Game& game, StaticFileRequestHandler&& static_handler)
        : game_{game}, static_file_handler_(std::move(static_handler)) {}

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    RequestHandler(RequestHandler&& other) : 
        game_(other.game_),
        static_file_handler_(std::move(other.static_file_handler_))  {}

    template <typename Body, typename Allocator, typename ResponseWriter>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& request, ResponseWriter&& writer) {
        if (!request.target().starts_with("/api/"s)){
            static_file_handler_(std::move(request), std::forward<ResponseWriter>(writer));
            return;
        }

        // отправить список карт
        if (request.target() == "/api/v1/maps"s){
            // получить список карт
            auto maps = game_.GetMaps();

            // спроецировать в DTO
            std::vector<dto::MapRegistryDto> maps_dto(maps.size());

            std::transform(maps.begin(), maps.end(), maps_dto.begin(), [](model::Map arg){return dto::MapRegistryDto(std::move(arg));});

            // отправить массив DTO
            writer(std::move(Json(request, maps_dto)));
            return;
        }

        // отправить карту
        if (request.target().starts_with("/api/v1/maps/"s)){
            auto map_name = std::string(request.target().substr("/api/v1/maps/"s.size()));

            auto map = game_.FindMap(model::Map::Id{map_name});

            if (map == nullptr){
                writer(std::move(Json(request, dto::ErrorDto {"mapNotFound"s, "Map not found"s}, http::status::not_found)));
                return;
            }

            writer(std::move(Json(request, map)));
            return;
        }

        if (request.target() == "/api/v1/game/join"s){
            JoinRequestHandler join_handler {game_};
            join_handler(std::move(request), writer);
            return;
        }

        if (request.target() == "/api/v1/game/players"s){
            GetPlayersRequestHandler get_players_handler{game_};
            get_players_handler(std::move(request), writer);
            return;
        }

        // отправить BadRequest
        writer(std::move(Json(request, dto::ErrorDto {"badRequest"s, "Bad request"s}, http::status::bad_request)));
    }

private:
    model::Game& game_;
    StaticFileRequestHandler static_file_handler_;
};

}  // namespace http_handler
