#include "audio.h"
#include <iostream>
#include "client.h"
#include "server.h"

using namespace std::literals;

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "Type and port required!" << std::endl;

        return 1;
    }

    std::string_view type(argv[1]);
    auto port = std::atoi(argv[2]);

    if (type == "client"sv){
        Client client;

        client.SendMessage(port);

        return 0;
    }

    if (type == "server") {
        Server server(port);

        server.Listen();

        return 0;
    }

    std::cout << "Invalid type: " << type << std::endl;

    return 1;

    // Recorder recorder(ma_format_u8, 1);
    // Player player(ma_format_u8, 1);

    // while (true) {
    //     std::string str;

    //     std::cout << "Press Enter to record message..." << std::endl;
    //     std::getline(std::cin, str);

    //     auto rec_result = recorder.Record(65000, 1.5s);
    //     std::cout << "Recording done" << std::endl;

    //     player.PlayBuffer(rec_result.data.data(), rec_result.frames, 1.5s);
    //     std::cout << "Playing done" << std::endl;
    // }

    return 0;
}
