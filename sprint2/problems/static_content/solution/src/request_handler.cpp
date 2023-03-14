#include "request_handler.h"
#include "json_loader.h"
#include <boost/json.hpp>

namespace json = boost::json;
using namespace std::literals;


namespace http_handler {

std::string_view mime_type(fs::path path)
{
    using beast::iequals;

    if (!path.has_extension())
        return "application/octet-stream";

    auto ext = path.extension().string();

    if(iequals(ext, ".htm"))  return "text/html";
    if(iequals(ext, ".html")) return "text/html";
    if(iequals(ext, ".php"))  return "text/html";
    if(iequals(ext, ".css"))  return "text/css";
    if(iequals(ext, ".txt"))  return "text/plain";
    if(iequals(ext, ".js"))   return "application/javascript";
    if(iequals(ext, ".json")) return "application/json";
    if(iequals(ext, ".xml"))  return "application/xml";
    if(iequals(ext, ".swf"))  return "application/x-shockwave-flash";
    if(iequals(ext, ".flv"))  return "video/x-flv";
    if(iequals(ext, ".png"))  return "image/png";
    if(iequals(ext, ".jpe"))  return "image/jpeg";
    if(iequals(ext, ".jpeg")) return "image/jpeg";
    if(iequals(ext, ".jpg"))  return "image/jpeg";
    if(iequals(ext, ".gif"))  return "image/gif";
    if(iequals(ext, ".bmp"))  return "image/bmp";
    if(iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
    if(iequals(ext, ".tiff")) return "image/tiff";
    if(iequals(ext, ".tif"))  return "image/tiff";
    if(iequals(ext, ".svg"))  return "image/svg+xml";
    if(iequals(ext, ".svgz")) return "image/svg+xml";
    return "application/octet-stream";
};

StaticFileRequestHandler::StaticFileRequestHandler(fs::path wwwroot) : wwwroot_(wwwroot) {};

std::string RequestHandler::GetMapsJson(){
    auto maps = game_.GetMaps();

    json::array json_array;

    for(const auto& map : maps){
        json::object jv;

        jv["id"s] = *map.GetId();
        jv["name"s] = map.GetName();

        json_array.emplace_back(jv);
    }

    auto result = json::serialize(json_array);

    return result;
}

std::string RequestHandler::GetMapJson(const model::Map* map){

    auto obj = json::value_from(map);

    return json::serialize(obj);
}

RequestHandler::JsonResponse RequestHandler::MakeJsonResponse(http::status status, unsigned http_version, std::string_view body){
    JsonResponse response(status, http_version);
    response.body() = body;
    response.content_length(response.body().size());
    response.set(http::field::content_type, "application/json");

    return response;
}

RequestHandler::JsonResponse RequestHandler::MakeErrorResponse(
    http::status status, 
    unsigned http_version, 
    std::string_view code, 
    std::string_view message){
    json::value jv({{"code", code}, {"message", message}});

    JsonResponse response(status, http_version);
    response.body() = json::serialize(jv);
    response.content_length(response.body().size());
    response.set(http::field::content_type, "application/json");

    return response;
}

}  // namespace http_handler
