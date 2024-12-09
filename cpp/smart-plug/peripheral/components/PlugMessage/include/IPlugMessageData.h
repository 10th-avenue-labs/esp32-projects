#ifndef IPLUG_MESSAGE_DATA_H
#define IPLUG_MESSAGE_DATA_H

#include <string>

using namespace std;

class IPlugMessageData {
    public:
        virtual ~IPlugMessageData() = default;
        virtual string serialize() = 0;
};

#endif // IPLUG_MESSAGE_DATA_H