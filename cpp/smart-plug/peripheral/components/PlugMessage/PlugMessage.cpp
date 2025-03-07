#include "PlugMessage.h"

map<string, function<unique_ptr<IPlugMessageData>(const string&)>> PlugMessage::deserializers = {};

void PlugMessage::registerDeserializer(const string& type, function<unique_ptr<IPlugMessageData>(const string&)> deserializer) {
    deserializers[type] = deserializer;
}

PlugMessage PlugMessage::deserialize(const string& serialized) {
    // Create a new configuration
    PlugMessage config;

    // Parse the serialized string
    unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_Parse(serialized.c_str()), cJSON_Delete);

    // Deserialize the object
    config.type = cJSON_GetObjectItem(root.get(), "type")->valuestring;

    function<unique_ptr<IPlugMessageData>(const string&)> deserializer = deserializers[config.type];

    if (deserializer == nullptr) {
        // TODO: Handle
        return config;
    }

    string serializedData = cJSON_PrintUnformatted(cJSON_GetObjectItem(root.get(), "data"));

    config.data = deserializer(serializedData);

    return config;
};

string PlugMessage::serialize() {
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", type.c_str());
    cJSON_AddItemToObject(root, "data", cJSON_Parse(data->serialize().c_str()));
    return cJSON_Print(root);
    // return cJSON_PrintUnformatted(root);
}