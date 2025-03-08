#include "Request.h"

// Initialize the static unordered_map
std::unordered_map<std::string, std::type_index> SmartDevice::Request::dataTypesByTypeName;

// Initialize a function to always return a nullptr
std::function<Result<std::unique_ptr<IDeserializable>>(const cJSON *root)> SmartDevice::Request::voidReturner =
    [](const cJSON *root) -> Result<std::unique_ptr<IDeserializable>>
{
    return Result<std::unique_ptr<IDeserializable>>::createSuccess(nullptr);
};