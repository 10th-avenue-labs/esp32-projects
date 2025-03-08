#include "IDeserializable.h"

// Initialize the static unordered_map
std::unordered_map<std::type_index, std::function<Result<std::unique_ptr<IDeserializable>>(const cJSON *root)>> IDeserializable::deserializers;
