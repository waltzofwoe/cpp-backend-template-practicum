#include "sdk.h"
//

#include <boost/asio/signal_set.hpp>
#include <boost/asio/io_context.hpp>
#include <iostream>
#include <thread>

#include "json_loader.h"
#include "request_handler.h"
#include "logger.h"
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/trivial.hpp>

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
namespace logs = boost::log;


namespace {

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
    if (argc != 3) {
        std::cerr << "Usage: game_server <game-config-json> <wwwroot>"sv << std::endl;
        return EXIT_FAILURE;
    }
    try {
        // 1. Загружаем карту из файла и построить модель игры
        model::Game game = json_loader::LoadGame(argv[1]);

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

        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        http_handler::RequestHandler handler{game, http_handler::StaticFileRequestHandler(argv[2])};

        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;

        http_server::ServeHttp(ioc, {address, port}, std::forward<http_handler::RequestHandler>(handler));
        
        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        std::cout << "Server has started..."sv << std::endl;

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
