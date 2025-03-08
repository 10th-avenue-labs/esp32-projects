#pragma once

#include "IDeserializable.h"
#include "ISerializable.h"

#include <string>

namespace SmartDevice
{
    class CloudConnectionConfig : public IDeserializable, public ISerializable
    {
    public:
        std::string deviceId;
        std::string jwt;
        std::string ssid;
        std::string password;
        std::string mqttConnectionString;

        /**
         * @brief Construct a new Cloud Connection Config object
         *
         * @param deviceId The device ID
         * @param jwt The JWT to use for connecting to the MQTT broker
         * @param ssid The SSID of the wifi network
         * @param password The password of the wifi network
         * @param mqttConnectionString The connection string for the MQTT broker
         */
        CloudConnectionConfig(std::string deviceId, std::string jwt, std::string ssid, std::string password, std::string mqttConnectionString)
            : deviceId(deviceId), jwt(jwt), ssid(ssid), password(password), mqttConnectionString(mqttConnectionString) {};

        /**
         * @brief Copy constructor
         *
         * @param other The other CloudConnectionConfig object
         */
        CloudConnectionConfig(const CloudConnectionConfig &other)
            : deviceId(other.deviceId), jwt(other.jwt), ssid(other.ssid), password(other.password), mqttConnectionString(other.mqttConnectionString) {};

        /**
         * @brief Serialize the object to a cJSON object
         *
         * @return std::unique_ptr<cJSON, void (*)(cJSON *item)> The serialized cJSON object
         */
        std::unique_ptr<cJSON, void (*)(cJSON *item)> serialize() override
        {
            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "deviceId", deviceId.c_str());
            cJSON_AddStringToObject(root, "jwt", jwt.c_str());
            cJSON_AddStringToObject(root, "ssid", ssid.c_str());
            cJSON_AddStringToObject(root, "password", password.c_str());
            cJSON_AddStringToObject(root, "mqttConnectionString", mqttConnectionString.c_str());
            return std::unique_ptr<cJSON, void (*)(cJSON *item)>(root, cJSON_Delete);
        }

        /**
         * @brief Deserialize the object from a cJSON object
         *
         * @param root The cJSON object
         * @return Result<std::unique_ptr<IDeserializable>> The deserialized object
         */
        static Result<std::unique_ptr<IDeserializable>> deserialize(const cJSON *root)
        {
            cJSON *deviceIdItem = cJSON_GetObjectItem(root, "deviceId");
            if (!cJSON_IsString(deviceIdItem))
            {
                return Result<std::unique_ptr<IDeserializable>>::createFailure("Invalid or missing deviceId field in JSON");
            }

            cJSON *jwtItem = cJSON_GetObjectItem(root, "jwt");
            if (!cJSON_IsString(jwtItem))
            {
                return Result<std::unique_ptr<IDeserializable>>::createFailure("Invalid or missing jwt field in JSON");
            }

            cJSON *ssidItem = cJSON_GetObjectItem(root, "ssid");
            if (!cJSON_IsString(ssidItem))
            {
                return Result<std::unique_ptr<IDeserializable>>::createFailure("Invalid or missing ssid field in JSON");
            }

            cJSON *passwordItem = cJSON_GetObjectItem(root, "password");
            if (!cJSON_IsString(passwordItem))
            {
                return Result<std::unique_ptr<IDeserializable>>::createFailure("Invalid or missing password field in JSON");
            }

            cJSON *mqttConnectionStringItem = cJSON_GetObjectItem(root, "mqttConnectionString");
            if (!cJSON_IsString(mqttConnectionStringItem))
            {
                return Result<std::unique_ptr<IDeserializable>>::createFailure("Invalid or missing mqttConnectionString field in JSON");
            }

            return Result<std::unique_ptr<IDeserializable>>::createSuccess(
                std::make_unique<CloudConnectionConfig>(
                    deviceIdItem->valuestring,
                    jwtItem->valuestring,
                    ssidItem->valuestring,
                    passwordItem->valuestring,
                    mqttConnectionStringItem->valuestring));
        }
    };
};