#include <iostream>
#include <string>

#include "SmartDevice.h"
#include "esp_log.h"
#include "esp_event.h"

class TestSmartDevice : public SmartDevice
{
};

extern "C" void app_main()
{

    TestSmartDevice device;
}