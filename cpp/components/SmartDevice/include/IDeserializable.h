#pragma once

#include "Result.h"

#include <esp_log.h>
#include <functional>
#include <typeindex>
#include <memory>
#include <unordered_map>
#include <cJSON.h>

static const char *IDESERIALIZABLE_TAG = "IDESERIALIZABLE";

class IDeserializable
{
public:
    virtual ~IDeserializable() = default;

    /**
     * @brief Register a deserializer for a type
     *
     * @param deserializer The function to deserialize the object
     */
    template <typename T>
    static void registerDeserializer(std::function<Result<std::unique_ptr<IDeserializable>>(const cJSON *root)> deserializer)
    {
        static_assert(std::is_base_of<IDeserializable, T>::value, "T must inherit from IDeserializable");

        if (deserializers.find(typeid(T)) != deserializers.end())
        {
            ESP_LOGW(IDESERIALIZABLE_TAG, "Deserializer already registered for type '%s', skipping registration", typeid(T).name());
            return;
        }

        deserializers[typeid(T)] = deserializer; // Use typeid(T) as the key
    }

    /**
     * @brief Get a deserializer for a given type
     *
     * @param type The type of the object to deserialize
     * @return std::function<Result<std::unique_ptr<IDeserializable>>(const cJSON *root)> The deserializer function
     */
    template <typename T>
    static std::function<Result<std::unique_ptr<IDeserializable>>(const cJSON *root)> getDeserializer()
    {
        auto it = deserializers.find(typeid(T)); // Use typeid(T) as the key
        if (it != deserializers.end())
        {
            return it->second;
        }
        return nullptr; // Or throw an exception if deserializer not found
    }

    /**
     * @brief Get the Deserializer for a particular type
     *
     * @param type The type to get the deserializer for
     * @return std::function<Result<std::unique_ptr<IDeserializable>>(const cJSON *root)> The deserializer function
     */
    static std::function<Result<std::unique_ptr<IDeserializable>>(const cJSON *root)> getDeserializer(const std::type_index &type)
    {
        auto it = deserializers.find(type);
        if (it != deserializers.end())
        {
            return it->second;
        }
        return nullptr; // Or throw an exception if deserializer not found
    }

private:
    static std::unordered_map<type_index, std::function<Result<std::unique_ptr<IDeserializable>>(const cJSON *root)>> deserializers;
};
