#pragma once

#include "audio.h"
#include <boost/asio.hpp>
#include <iostream>
#include <vector>

namespace net = boost::asio;

using udp = net::ip::udp;
using namespace std::literals;

class Server {
    public:
        Server(int port) : 
            _port(port), 
            _player(ma_format_u8, 1),
            _io_context() {}

        void Listen(){

            udp::socket socket(_io_context, udp::endpoint(udp::v4(), _port));

            while(true)
            {
                auto message = getMessage(socket);

                play(message);
            }
        }

    private:
        int _port;
        Player _player;
        net::io_context _io_context;

        Recorder::RecordingResult getMessage(udp::socket &socket){

            static const size_t max_buffer_size = 1024*64;

            std::vector<char> recv_buf(max_buffer_size);

            udp::endpoint remote_endpoint;

            std::cout<< "Waiting for message" << std::endl;

            auto size = socket.receive_from(net::buffer(recv_buf), remote_endpoint);

            Recorder::RecordingResult result;

            result.data = recv_buf;

            result.frames = size / _player.GetFrameSize();

            std::cout << "Received message, frames count = " << result.frames << std::endl;;

            return result;
        }

        void play(Recorder::RecordingResult rec_result){
            std::cout << "Begin playing" << std::endl;
            _player.PlayBuffer(rec_result.data.data(), rec_result.frames, 1.5s);
            std::cout << "Playing done" << std::endl;
        }
};