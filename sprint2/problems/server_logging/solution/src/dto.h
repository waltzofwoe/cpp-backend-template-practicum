#pragma once
#include <string>
#include "model.h"

namespace dto {

/// @brief DTO для отображения списка карт
struct MapRegistryDto {
    std::string Id;
    std::string Name;

    MapRegistryDto() {};    

    MapRegistryDto(model::Map&& map) :
        Id(*map.GetId()),
        Name(map.GetName()){}
}; // struct MapRegistryDto

/// @brief DTO для отображения ошибки
struct ErrorDto {
    std::string Code;
    std::string Message;
}; //struct ErrorDto
} // namespace dto