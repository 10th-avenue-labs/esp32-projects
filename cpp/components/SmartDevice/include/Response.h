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

        /**
         * @brief Construct a new Response object
         *
         * @param type The type of the response
         * @param data The serializable data to include in the response
         */
        Response(std::string type, std::unique_ptr<ISerializable> data) : type(type), data(std::move(data)) {}

        /**
         * @brief Serialize the object to a cJSON object
         *
         * @return std::unique_ptr<cJSON, void (*)(cJSON *item)> The serialized cJSON object
         */
        std::unique_ptr<cJSON, void (*)(cJSON *item)> serialize() override
        {
            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "type", type.c_str());
            cJSON_AddItemToObject(root, "data", data->serialize().release());
            return std::unique_ptr<cJSON, void (*)(cJSON *item)>(root, cJSON_Delete);
        }
    };
}