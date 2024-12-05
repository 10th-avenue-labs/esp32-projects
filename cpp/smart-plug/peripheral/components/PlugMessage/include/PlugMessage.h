#ifndef PLUG_MESSAGE_H
#define PLUG_MESSAGE_H

#include <string>
#include <cJSON.h>
#include <memory>
#include "IPlugMessageData.h"
#include <map>
#include <functional>

using namespace std;

class PlugMessage {
    public:
        string type;
        unique_ptr<IPlugMessageData> message;

        static void registerDeserializer(const string& type, function<unique_ptr<IPlugMessageData>(const string&)> deserializer);

        static PlugMessage deserialize(const string& serialized);
    private:
        static map<string, function<unique_ptr<IPlugMessageData>(const string&)>> deserializers;
};

#endif // PLUG_MESSAGE_H