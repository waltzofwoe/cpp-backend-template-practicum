#include "http_server.h"
#include "logger.h"

#include <boost/asio/dispatch.hpp>
#include <iostream>

namespace http_server {

    void ReportError(beast::error_code ec, std::string_view where)
    {
        json::value error_data {
            {"code"s, ec.value()},
            {"text"s, ec.message()},
            {"where"s, where}
        };

        logger::Error("error", error_data);
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

        request_time_ = chrono::system_clock::now();

        HandleRequest(std::move(request_));
    }

    tcp::endpoint SessionBase::GetEndpoint() const{
        return stream_.socket().local_endpoint();
    }

    const SessionBase::HttpRequest& SessionBase::GetRequest() const {
        return request_;
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
        stream_.socket().shutdown(tcp::socket::shutdown_send);
    }
}  // namespace http_server
