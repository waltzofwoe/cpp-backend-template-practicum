#define BOOST_BEAST_USE_STD_STRING_VIEW

#include<boost/asio.hpp>
#include<boost/beast.hpp>
#include<thread>
#include<iostream>
#include<string>

namespace net = boost::asio;
using  tcp = net::ip::tcp;
using namespace std::literals;
namespace beast = boost::beast;
namespace http = beast::http;

using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;

class ConnectionHandler{
    public: 
    void Handle(tcp::socket& socket) {
        beast::flat_buffer buffer;

        while(auto request = ReadRequest(socket, buffer)){
            SendResponse(*request, socket);
        }
    }

    private:
    std::optional<StringRequest> ReadRequest(tcp::socket& socket, beast::flat_buffer& buffer){
        beast::error_code ec;

        StringRequest request;

        http::read(socket, buffer, request, ec);

        if(ec == http::error::end_of_stream){
            return std::nullopt;
        }

        if (ec)
        {
            throw std::runtime_error("Failed to read request: "s.append(ec.message()));
        }

        return request;
    }

    void SendResponse(const StringRequest& request, tcp::socket& socket){
        if (request.method() != http::verb::get && 
            request.method() != http::verb::head) 
        {
            StringResponse response(http::status::method_not_allowed, request.version());
            response.set(http::field::content_type, "text/html"sv);
            response.set(http::field::allow, "GET, HEAD");
            response.body() = "Invalid method"sv;
            response.content_length(response.body().size());
            response.keep_alive(request.keep_alive());

            http::write(socket, response);
        }

        StringResponse response(http::status::ok, request.version());

        response.body() = "Hello, "s.append(request.target().substr(1));

        response.set(http::field::content_type, "text/html"sv);

        response.content_length(response.body().size());
        response.keep_alive(request.keep_alive());

        http::write(socket, response);
    }
};

class Server {
    public: 

    Server(std::string address, short unsigned int port) : _address(address), _port(port){}
    
    void Run(){
        net::io_context context;

        const auto address = net::ip::make_address(_address);

        tcp::acceptor acceptor(context, {address, _port});

        std::cout << "Server has started..."sv << std::endl;

        while(true){
            tcp::socket socket(context);

            acceptor.accept(socket);

            // std::thread t([this](tcp::socket socket){
            //     HandleConnection(socket);
            // },
            // std::move(socket));

            HandleConnection(socket);
        }

    }

    private:

    void HandleConnection(tcp::socket& socket){
        try {
            _handler.Handle(socket);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }

        beast::error_code ec;

        socket.shutdown(tcp::socket::shutdown_send, ec);
    }

    short unsigned int _port;
    std::string _address;

    ConnectionHandler _handler;
};

