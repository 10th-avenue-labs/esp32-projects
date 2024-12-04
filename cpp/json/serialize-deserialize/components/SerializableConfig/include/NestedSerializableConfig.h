#ifndef NESTED_SERIALIZABLE_CONFIG_H
#define NESTED_SERIALIZABLE_CONFIG_H

#include <string>
#include "cJSON.h"
#include <memory>

using namespace std;

// This is an example of a class that can be serialized
class NestedSerializableConfig {
    public:
        int anInt;
        float aFloat;
        bool aBool;
        string aString;
        unique_ptr<cJSON, void (*)(cJSON *item)> serialize();

        static NestedSerializableConfig deserialize(const string &serialized);
};

#endif // NESTED_SERIALIZABLE_CONFIG_H