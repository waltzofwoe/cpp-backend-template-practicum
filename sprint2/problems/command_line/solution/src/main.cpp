#include "sdk.h"
//

#include <boost/asio/signal_set.hpp>
#include <boost/asio/io_context.hpp>
#include <iostream>
#include <thread>

#include "json_loader.h"
#include "request_handler.h"
#include "logger.h"
#include "application.h"
#include "ticker.h"
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/trivial.hpp>
#include <boost/program_options.hpp>

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
namespace logs = boost::log;


namespace {

struct Args {
    int tick_period;
    bool has_tick_period;
    std::string config_file;
    std::string www_root;
    bool randomize_spawn_points;

    Args() : tick_period{0}, config_file{}, www_root{}, randomize_spawn_points{false} {};
};

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    namespace po = boost::program_options;

    po::options_description desc {"All options"s};

    Args args;

    desc.add_options()           //
        ("help,h", "produce help message")  //
        ("tick-period,t", po::value(&args.tick_period)->value_name("milliseconds"s), "set tick period")
        ("config-file,c", po::value(&args.config_file)->value_name("file"s)->required(), "set config file path")
        ("www-root,w", po::value(&args.www_root)->value_name("dir"s)->required(), "set static files root")
        ("randomize-spawn-points", "spawn dogs at random positions");

    po::variables_map vm;

    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s) || !vm.contains("config-file") || !vm.contains("www-root")) {
        std::cout << desc;
        return std::nullopt;
    }

    if (args.has_tick_period = vm.contains("tick-period")){
        if (args.tick_period <= 0)
        {
            throw std::runtime_error("invalid tick-period value");
        }
    };

    args.randomize_spawn_points = vm.contains("randomize-spawn-points");

    return args;
} 

// Запускает функцию fn на workers_count потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned workers_count, const Fn& fn) {
    workers_count = std::max(1u, workers_count);
    std::vector<std::jthread> workers;
    workers.reserve(workers_count - 1);
    // Запускаем workers_count-1 рабочих потоков, выполняющих функцию fn
    while (--workers_count) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace

int main(int argc, const char* argv[]) {
    auto args = ParseCommandLine(argc, argv);

    if (!args) {
        return EXIT_FAILURE;
    }
    try {
        // 1. Загружаем карту из файла и построить модель игры
        auto game = json_loader::LoadGame(args->config_file);

        app::Application application { game, args->randomize_spawn_points};

        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM

        net::signal_set signals(ioc, SIGINT, SIGTERM);

        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
            }
        });

        auto apiStrand = net::make_strand(ioc);

        auto timer = std::make_shared<app::ApplicationUpdateTimer>(apiStrand, application, std::chrono::milliseconds(args->tick_period));

        if (args->has_tick_period){
            timer->Start();
        }

        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игр

        auto handler = std::make_shared<http_handler::RequestHandler>(http_handler::ApiHandler {application, args->has_tick_period}, http_handler::StaticFileRequestHandler(args->www_root), apiStrand);

        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;

        http_server::ServeHttp(ioc, {address, port}, [handler](auto&& request, auto&& writer){
            (*handler)(std::forward<decltype(request)>(request), std::forward<decltype(writer)>(writer));
        });
        
        // инициализация логгера
        logger::InitBoostLogs();

        json::value custom_data {
            {"port"s, port},
            {"address"s, address.to_string()}
        };

        // Запись о том, что сервер запущен и готов обрабатывать запросы
        logger::Info("server started"s, custom_data);

        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });

        logger::Info("server exited"s, { {"code", 0 }});
    } catch (const std::exception& ex) {
        
        json::value custom_data {
            {"code"s, EXIT_FAILURE},
            {"exception"s, ex.what()}
        };

        logger::Fatal("server exited"s, custom_data);

        return EXIT_FAILURE;
    }
}
