#pragma once

#include "audio.h"
#include <boost/asio.hpp>
#include <string>
#include <sstream>
#include <iostream>

namespace net = boost::asio;

using net::ip::udp;
using namespace std::literals;

class Client
{
public:
    Client() : _recorder(ma_format_u8, 1)
    {
    }

    void SendMessage(const int port)
    {
        auto rec_result = getRecord();

        std::string address;

        std::cout << "Please enter address" << std::endl;

        std::getline(std::cin, address);

        sendMessage(address, port, rec_result);
    }

private:
    Recorder _recorder;

    Recorder::RecordingResult getRecord()
    {
        auto rec_result = _recorder.Record(65000, 1.5s);

        std::cout << "Frames count: " << rec_result.frames << std::endl;

        return rec_result;
    }

    void sendMessage(const std::string &address, int port, Recorder::RecordingResult &record)
    {
        net::io_context io_context;

        udp::socket socket(io_context, udp::v4());

        auto endpoint = udp::endpoint(net::ip::make_address(address), port);

        auto bufferSize = _recorder.GetFrameSize() * record.frames;

        socket.send_to(net::buffer(record.data, bufferSize), endpoint);

        std::cout << "Sent message to server";
    }
};