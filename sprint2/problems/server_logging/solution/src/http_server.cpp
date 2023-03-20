#include "http_server.h"

#include <boost/asio/dispatch.hpp>
#include <iostream>

namespace http_server {

    void ReportError(beast::error_code ec, std::string_view what)
    {
        std::cerr << what << ": "sv << ec.message() << std::endl;
    }

    void SessionBase::Run()
    {
        net::dispatch(stream_.get_executor(),
                      beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
    }

    void SessionBase::Read()
    {
        request_ = {};

        stream_.expires_after(30s);

        http::async_read(stream_, buffer_, request_,
                         beast::bind_front_handler(&SessionBase::OnRead, this->GetSharedThis()));
    };

    void SessionBase::OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read)
    {
        if (ec == http::error::end_of_stream)
        {
            return Close();
        }

        if (ec)
        {
            return ReportError(ec, "read"sv);
        }

        HandleRequest(std::move(request_));
    }

    tcp::endpoint SessionBase::GetEndpoint() const{
        return stream_.socket().local_endpoint();
    }

    const SessionBase::HttpRequest& SessionBase::GetRequest() const {
        return request_;
    }

    void SessionBase::Write(http::message_generator&& response)
    {
        bool keep_alive = response.keep_alive();

        beast::async_write(stream_, 
            std::move(response),
            beast::bind_front_handler(&SessionBase::OnWrite, GetSharedThis(), keep_alive));
    }

    void SessionBase::OnWrite(bool keep_alive, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written)
    {
        if (ec)
        {
            return ReportError(ec, "write"sv);
        }

        if (!keep_alive)
        {
            return Close();
        }

        Read();
    }

    void SessionBase::Close() {
        beast::error_code ec;

        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
    }
}  // namespace http_server
