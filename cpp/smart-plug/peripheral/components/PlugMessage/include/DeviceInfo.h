#ifndef DEVICE_INFO_H
#define DEVICE_INFO_H

#include <string>
#include <ISerializable.h>

using namespace std;

class DeviceInfo : public ISerializable {
    public:
        string name;
        string type;

        DeviceInfo(string name, string type) : name(name), type(type) {}

        string serialize() {
            cJSON* root = cJSON_CreateObject();

            cJSON_AddStringToObject(root, "name", name.c_str());
            cJSON_AddStringToObject(root, "type", type.c_str());

            return cJSON_PrintUnformatted(root);
        }
};

#endif // DEVICE_INFO_H