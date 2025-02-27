#pragma once

#include "ISerializable.h"
#include "Result.h"

namespace SmartDevice
{
    class Response : public ISerializable
    {
    public:
        std::string type;
        std::unique_ptr<ISerializable> data;

        Response(std::string type, std::unique_ptr<ISerializable> data) : type(type), data(std::move(data)) {}

        std::unique_ptr<cJSON, void (*)(cJSON *item)> serialize() override
        {
            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "type", type.c_str());
            cJSON_AddItemToObject(root, "data", data->serialize().release());
            return std::unique_ptr<cJSON, void (*)(cJSON *item)>(root, cJSON_Delete);
        }
    };
}