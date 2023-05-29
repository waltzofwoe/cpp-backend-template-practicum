#pragma once

#include <random>
#include <sstream>
#include <iomanip>

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

        std::ostringstream str;

        str << std::setfill ('0') << std::setw(sizeof(uint))<< std::hex << value1 << value2;

        return str.str();
    }
}