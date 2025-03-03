#pragma once
#include "sdk.h"
//
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#define BOOST_BEAST_USE_STD_STRING_VIEW

namespace http_server {

namespace net = boost::asio;
using tcp = net::ip::tcp;
namespace beast = boost::beast;
namespace http = beast::http;

using namespace std::literals;

void ReportError(beast::error_code ec, std::string_view what); 

class SessionBase {
public:
    SessionBase(const SessionBase&) = delete;
    SessionBase& operator=(const SessionBase&) = delete;

    void Run();

protected:
    using HttpRequest = http::request<http::string_body>;

    explicit SessionBase(tcp::socket&& socket) : stream_(std::move(socket)){}

    ~SessionBase() = default;

    void Write(http::message_generator &&response);

private:
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    HttpRequest request_;

    void Read();

    void OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read);

    void OnWrite(bool keep_alive, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written);

    void Close();

    virtual void HandleRequest(HttpRequest&& request) = 0;

    virtual std::shared_ptr<SessionBase> GetSharedThis() = 0;
};

template <typename RequestHandler>
class Session : public SessionBase, public std::enable_shared_from_this<Session<RequestHandler>> {
public:
    Session(tcp::socket&& socket, std::shared_ptr<RequestHandler> handler)
        : SessionBase(std::move(socket)),
          request_handler_(handler){
    }

private:
    std::shared_ptr<RequestHandler> request_handler_;

    std::shared_ptr<SessionBase> GetSharedThis() override {
        return this->shared_from_this();
    }

    void HandleRequest(HttpRequest&& request) override {
        Write((*request_handler_)(std::move(request)));
    }
};

template <typename RequestHandler>
class Listener : public std::enable_shared_from_this<Listener<RequestHandler>> {
public:
    Listener(net::io_context& ioc, const tcp::endpoint& endpoint, std::shared_ptr<RequestHandler> request_handler)
        : ioc_(ioc),
          acceptor_(net::make_strand(ioc)),
          request_handler_(request_handler) {
        acceptor_.open(endpoint.protocol());

        acceptor_.set_option(net::socket_base::reuse_address(true));

        acceptor_.bind(endpoint);

        acceptor_.listen(net::socket_base::max_listen_connections);
    }

    void Run(){
        DoAccept();
    }
    
private:
    net::io_context& ioc_;
    tcp::acceptor acceptor_{};
    std::shared_ptr<RequestHandler> request_handler_;

    void DoAccept() {
        acceptor_.async_accept(net::make_strand(ioc_), 
            beast::bind_front_handler(&Listener::OnAccept, this->shared_from_this()));
    }

    void OnAccept(beast::error_code ec, tcp::socket socket){
        if (ec) {
            return ReportError(ec, "accept"sv);
        }

        AsyncRunSession(std::move(socket));

        DoAccept();
    }

    void AsyncRunSession(tcp::socket&& socket){
        std::make_shared<Session<RequestHandler>>(std::move(socket), request_handler_)->Run();
    }
};

template <typename RequestHandler>
void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, std::shared_ptr<RequestHandler> handler) {
    using MyListener = Listener<std::decay_t<RequestHandler>>;

    std::make_shared<MyListener>(ioc, endpoint, handler)->Run();
}

}  // namespace http_server
