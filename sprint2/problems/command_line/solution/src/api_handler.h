#pragma once

#include "application.h"
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <regex>

#define BOOST_BEAST_USE_STD_STRING_VIEW

using namespace std::literals;

namespace http = boost::beast::http;
namespace json = boost::json;

namespace http_handler {
    using StringRequest = http::request<http::string_body>;
    using JsonResponse = http::response<http::string_body>;

    template <typename Body, typename Allocator, typename Object>
    JsonResponse Json(
        const http::request<Body, http::basic_fields<Allocator>>& request, 
        Object object,
        http::status status_code = http::status::ok){
        auto jv = json::value_from(object);

        auto json_string = json::serialize(jv);

        JsonResponse response {status_code, request.version()};
        response.set(http::field::content_type, "application/json");
        response.set(http::field::cache_control, "no-cache");
        response.body() = json_string;
        response.keep_alive(request.keep_alive());
        response.prepare_payload();
        return response;
    }

    template <typename Body, typename Allocator>
    JsonResponse Json(
        const http::request<Body, http::basic_fields<Allocator>>& request, 
        json::value jvalue,
        http::status status_code = http::status::ok)
    {
        auto json_string = json::serialize(jvalue);
        JsonResponse response {status_code, request.version()};
        response.set(http::field::content_type, "application/json");
        response.body() = json_string;
        response.keep_alive(request.keep_alive());
        response.prepare_payload();
        return response;
    }

    JsonResponse HandleGetMaps(const app::Application& application, StringRequest&& request);

    JsonResponse HandleGetMapByName(app::Application& application, StringRequest&& request, const std::string& mapName);

    JsonResponse HandleJoinGame(app::Application& application, StringRequest&& request);

    JsonResponse HandleGetPlayers(app::Application& application, StringRequest&& request);

    JsonResponse HandleBadRequest(StringRequest&& request);

    JsonResponse HandleGetGameState(app::Application& application, StringRequest&& request);

    JsonResponse HandlePostPlayerAction(app::Application& application, StringRequest&& request);

    JsonResponse HandlePostGameTick(app::Application& application, StringRequest&& request);

    class ApiHandler {
        app::Application& _application;
        bool _disableTick;

        public:
        ApiHandler(const ApiHandler&) = delete;
        ApiHandler& operator=(const ApiHandler&) = delete;

        ApiHandler(ApiHandler&& other) : _application (other._application) {};

        explicit ApiHandler(app::Application& app, bool disableTick) : _application {app} {};
        
        bool IsApiRequest(std::string path) {
            return path.starts_with("/api/"s);
        }

        template<typename Body, typename Allocator, typename ResponseWriter>
        void operator()(http::request<Body, Allocator>&& request, ResponseWriter&& writer){
            std::string target = request.target();

            if (target == "/api/v1/maps"s){
                auto response = HandleGetMaps(_application, std::move(request));

                writer(response);

                return;
            }

            std::smatch smatch;

            if (std::regex_search(target, smatch, std::regex("/api/v1/maps/(\\w+)/?$"))){
                auto mapName = smatch.str(1);

                auto response = HandleGetMapByName(_application, std::move(request), mapName);

                writer(response);

                return;
            }

            if (request.target() == "/api/v1/game/join"s){
                auto response = HandleJoinGame(_application, std::move(request));

                writer(response);

                return;
            }

            if (request.target() == "/api/v1/game/players"s){
                auto response = HandleGetPlayers(_application, std::move(request));

                writer(response);

                return;
            }

            if (request.target() == "/api/v1/game/state"s){
                auto response = HandleGetGameState(_application, std::move(request));

                writer(response);

                return;
            }

            if(request.target() == "/api/v1/game/player/action"s) {
                auto response = HandlePostPlayerAction(_application, std::move(request));

                writer(response);

                return;
            }

            if (request.target() == "/api/v1/game/tick"s && !_disableTick) {
                auto response = HandlePostGameTick(_application, std::move(request));

                writer(response);
                
                return;
            }

            // отправить BadRequest
            auto response = HandleBadRequest(std::move(request));

            writer(response);
        }
    };
}