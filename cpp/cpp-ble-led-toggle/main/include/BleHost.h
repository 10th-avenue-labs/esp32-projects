#ifndef BLE_HOST
#define BLE_HOST

#include <esp_err.h>
#include <nvs_flash.h>
#include <esp_log.h>
#include <string>
#include "Result.h"

class BleHost
{
    public:
        BleHost();
        Result<esp_err_t> initialize();
};

#endif