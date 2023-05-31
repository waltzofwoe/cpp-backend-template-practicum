#pragma once

#include <filesystem>
#include <iostream>
#include <memory>
#include "http_server.h"
#include "model.h"
#include "dto.h"
#include "file_utils.h"
#include "logger.h"
#include "api_handler.h"

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
namespace net = boost::asio;

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

class RequestHandler :  public std::enable_shared_from_this<RequestHandler> {
public:

    using JsonResponse = http::response<http::string_body>;
    using StatusCodeResponse = http::response<http::empty_body>;
    using FileResponse = http::response<http::file_body>;
    using Strand = boost::asio::strand<boost::asio::io_context::executor_type>;

    explicit RequestHandler(ApiHandler&& apiHandler, StaticFileRequestHandler&& staticHandler, Strand strand) :
        _apiHandler{std::forward<ApiHandler>(apiHandler)}, 
        _staticFileHandler(std::forward<StaticFileRequestHandler>(staticHandler)),
        _strand {strand} {}
        

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    RequestHandler(RequestHandler&& other) : 
        _apiHandler(std::forward<ApiHandler>(other._apiHandler)),
        _staticFileHandler(std::forward<StaticFileRequestHandler>(other._staticFileHandler)),
        _strand { other._strand} {}

    template <typename Body, typename Allocator, typename ResponseWriter>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& request, ResponseWriter&& writer) {
        if (_apiHandler.IsApiRequest(request.target())){
            auto handle = [self = shared_from_this(), req = std::forward<decltype(request)>(request), 
                           writer] () mutable {
                assert(self->_strand.running_in_this_thread());
                self->_apiHandler(std::move(req), writer);
            };

            net::dispatch(_strand, handle);

            return;
        }
        _staticFileHandler(std::move(request), std::forward<ResponseWriter>(writer));
    }

private:
    ApiHandler _apiHandler;
    StaticFileRequestHandler _staticFileHandler;
    Strand _strand;
};

}  // namespace http_handler
