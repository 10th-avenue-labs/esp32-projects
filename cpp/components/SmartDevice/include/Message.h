#pragma once

#include "IDeserializable.h"
#include "Result.h"

#include "esp_log.h"
#include <string>
#include <unordered_map>

static const char *MESSAGE_TAG = "MESSAGE";

namespace SmartDevice
{
    class Message : public IDeserializable
    {
    public:
        std::string type;
        std::unique_ptr<IDeserializable> data;

        Message(std::string type, std::unique_ptr<IDeserializable> data) : type(type), data(std::move(data)) {}

        static void registerMessageType(std::string typeName, std::type_index typeIndex)
        {
            dataTypesByTypeName.emplace(typeName, typeIndex);
        }

        static Result<std::unique_ptr<IDeserializable>> deserialize(const cJSON *root)
        {
            // Get the fields
            cJSON *typeItem = cJSON_GetObjectItem(root, "type");
            cJSON *dataItem = cJSON_GetObjectItem(root, "data");

            // Verify the fields
            if (!cJSON_IsString(typeItem) || !cJSON_IsObject(dataItem))
            {
                return Result<std::unique_ptr<IDeserializable>>::createFailure("Invalid or missing fields in JSON");
            }

            // Get the type
            std::string type = typeItem->valuestring;
            ESP_LOGI(MESSAGE_TAG, "message type: %s", type.c_str());

            // Get the deserializer for the data type
            auto it = dataTypesByTypeName.find(type);
            if (it == dataTypesByTypeName.end())
            {
                return Result<std::unique_ptr<IDeserializable>>::createFailure("Failed to get data type for message type '" + type + "'");
            }
            ESP_LOGI(MESSAGE_TAG, "data type: %s", it->second.name());
            auto dataDeserializer = IDeserializable::getDeserializer(it->second);
            if (!dataDeserializer)
            {
                return Result<std::unique_ptr<IDeserializable>>::createFailure("Failed to get data deserializer");
            }

            // Deserialize the data
            auto deserializedDataResult = dataDeserializer(dataItem);
            if (!deserializedDataResult.isSuccess())
            {
                return Result<std::unique_ptr<IDeserializable>>::createFailure("Failed to deserialize data: " + deserializedDataResult.getError());
            }

            // Transfer ownership from the result
            auto dataValue = deserializedDataResult.getValue();
            if (!dataValue)
            {
                return Result<std::unique_ptr<IDeserializable>>::createFailure("Deserialized data is null");
            }

            // Return the deserialized message
            return Result<std::unique_ptr<IDeserializable>>::createSuccess(std::make_unique<Message>(type, std::move(dataValue)));

            // return Result<std::unique_ptr<IDeserializable>>::createFailure("Failed to deserialize data: ");
        }

    private:
        static std::unordered_map<std::string, std::type_index> dataTypesByTypeName;
    };
};