#pragma once
#include <random>
#include <sstream>
#include <iomanip>
#include <format>

namespace tokens {
    class token_generator
    {
    private:
        std::random_device _random_device;
        
        std::mt19937_64 _generator1 {[this] {
            std::uniform_int_distribution<std::mt19937_64::result_type> dist;
            return dist(_random_device);
        }()};

        std::mt19937_64 _generator2 {[this]{
            std::uniform_int_distribution<std::mt19937_64::result_type> dist;
            return dist(_random_device);
        }()};

        /* data */
    public:
        std::string create();
    };
    
    std::string token_generator::create(){
        auto value1 = _generator1();
        auto value2 = _generator2();

        return std::format("{:16x}{:16x}", value1, value2);
    }
}