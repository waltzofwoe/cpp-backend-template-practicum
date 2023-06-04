#pragma once

#include <memory>
#include <boost/asio.hpp>
#include "application.h"

namespace app {
    namespace net = boost::asio;
    namespace chrono = std::chrono;
    namespace sys = boost::system;

    class ApplicationUpdateTimer: public std::enable_shared_from_this<ApplicationUpdateTimer> {
        using Strand = net::strand<net::io_context::executor_type>;

        Strand _strand;
        Application& _application;
        chrono::milliseconds _updatePeriod;
        net::steady_timer _timer {_strand};

        void Tick();

        void ScheduleTick();

        public:
        ApplicationUpdateTimer(Strand strand, Application& application, chrono::milliseconds updatePeriod) :
            _strand {strand}, _application {application}, _updatePeriod {updatePeriod} {};

        void Start();
    };
}

