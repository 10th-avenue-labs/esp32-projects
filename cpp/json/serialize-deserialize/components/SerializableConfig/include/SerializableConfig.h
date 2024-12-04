#ifndef SERIALIZABLE_CONFIG_H
#define SERIALIZABLE_CONFIG_H

#include "NestedSerializableConfig.h"
#include <string>
#include "cJSON.h"
#include <memory>
#include <vector>

using namespace std;

// This is an example of a class that can be serialized
class SerializableConfig
{
    public:
        int anInt;
        float aFloat;
        bool aBool;
        string aString;
        vector<string> aStringVector;
        NestedSerializableConfig nestedConfig;

        unique_ptr<cJSON, void (*)(cJSON *item)> serialize();
        static SerializableConfig deserialize(const string &serialized);
};

#endif // SERIALIZABLE_CONFIG_H