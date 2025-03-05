#ifndef RESULT_H
#define RESULT_H

#include <optional>
#include <string>
#include <type_traits>
#include <esp_log.h>
#include "ISerializable.h"
#include <cJSON.h>
#include <memory>

using namespace std;

static const char *RESULT_TAG = "RESULT";

// Primary template for Result with a value of type T
template <typename T = void>
class Result : ISerializable
{
public:
    bool success;           // Flag indicating success or failure
    optional<T> value;      // Optional value for success
    optional<string> error; // Optional error message for failure

    /**
     * @brief Create a success result with a value
     *
     * @param value Value to store in the result
     * @return Result The result object
     */
    static Result createSuccess(T value)
    {
        return Result(true, value);
    }

    /**
     * @brief Create a failure result with an error message
     *
     * @param err Error message to store in the result
     * @return Result The result object
     */
    static Result createFailure(const string &err)
    {
        return Result(false, nullopt, err);
    }

    /**
     * @brief Check if the result is successful
     *
     * @return true If the result is successful
     * @return false If the result is a failure
     */
    bool isSuccess() const
    {
        return success;
    }

    /**
     * @brief Get the value stored in the result
     *
     * @return T The value stored in the result
     */
    T getValue() const
    {
        if (!success || !value)
        {
            ESP_LOGE(RESULT_TAG, "attempted to get value from failed result");
        }
        return *value;
    }

    /**
     * @brief Get the error message stored in the result
     *
     * @return string The error message stored in the result
     */
    string getError() const
    {
        if (success || !error)
        {
            ESP_LOGE(RESULT_TAG, "attempted to get error from successful result");
        }
        return *error;
    }

    unique_ptr<cJSON, void (*)(cJSON *item)> serialize() override
    {
        // Create a new cJSON object
        unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_CreateObject(), cJSON_Delete);

        // Serialize the object
        cJSON_AddItemToObject(root.get(), "success", cJSON_CreateBool(success));
        cJSON_AddItemToObject(root.get(), "error", error.has_value() ? cJSON_CreateString(error.value().c_str()) : cJSON_CreateNull());

        // Check if a value is present
        if (!value.has_value())
        {
            // No value is present, add a null value
            cJSON_AddItemToObject(root.get(), "value", cJSON_CreateNull());
            return root;
        }

        // Check if the value is a boolean
        if constexpr (is_same<T, bool>::value || is_same<T, bool>::value)
        {
            cJSON_AddItemToObject(root.get(), "value", cJSON_CreateBool(value.value()));
        }

        // Check if the value is a number
        else if constexpr (is_arithmetic<T>::value)
        {
            cJSON_AddItemToObject(root.get(), "value", cJSON_CreateNumber(value.value()));
        }

        // Check if the value is a string
        else if constexpr (is_same<T, string>::value)
        {
            cJSON_AddItemToObject(root.get(), "value", cJSON_CreateString(value.value().c_str()));
        }

        // Check if the value implements ISerializable
        else if constexpr (is_base_of<ISerializable, T>::value)
        {
            cJSON_AddItemToObject(root.get(), "value", value.value().serialize().release());
        }

        // Check if the type is a shared pointer to a type that implements ISerializable
        // TODO: We should have some more kind of generic serialization for shared pointers, in fact, this whole function could use a more generic re-write
        else if constexpr (is_same<T, shared_ptr<ISerializable>>::value)
        {
            cJSON_AddItemToObject(root.get(), "value", value.value() == nullptr ? cJSON_CreateNull() : value.value()->serialize().release());
        }

        // The value is not serializable
        else
        {
            ESP_LOGW(RESULT_TAG, "attempted to serialize a non-serializable type %s", typeid(T).name());
        }

        return root;
    }

private:
    /**
     * @brief Construct a new Result object
     *
     * @param success Flag indicating success or failure
     * @param value Optional value to store in the result
     * @param error Optional error message to store in the result
     */
    Result(bool success, optional<T> value = nullopt, optional<string> error = nullopt)
        : success(success), value(value), error(error) {}
};

// Specialization of Result<> when the value type is std::unique_ptr<T>
template <typename T>
class Result<std::unique_ptr<T>> : ISerializable
{
public:
    bool success;
    optional<std::unique_ptr<T>> value;
    optional<string> error;

    static Result createSuccess(std::unique_ptr<T> value)
    {
        return Result(true, std::move(value));
    }

    static Result createFailure(const string &err)
    {
        return Result(false, nullptr, err);
    }

    bool isSuccess() const
    {
        return success;
    }

    /**
     * @brief Get the Value object
     * Note: This function will transfer ownership of the value to the caller. Subsequent calls to this function will return nullptr
     *
     * @return std::unique_ptr<T> The value object
     */
    std::unique_ptr<T> getValue()
    {
        if (success && value.has_value())
        {
            return std::move(*value); // Only move if value is present
        }
        return nullptr; // Or handle the empty case appropriately
    }

    string getError() const
    {
        if (success || !error)
        {
            ESP_LOGE(RESULT_TAG, "attempted to get error from successful result");
        }
        return *error;
    }

    unique_ptr<cJSON, void (*)(cJSON *item)> serialize() override
    {
        // Create a new cJSON object
        unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_CreateObject(), cJSON_Delete);

        // Serialize the object
        cJSON_AddItemToObject(root.get(), "success", cJSON_CreateBool(success));
        cJSON_AddItemToObject(root.get(), "error", error.has_value() ? cJSON_CreateString(error.value().c_str()) : cJSON_CreateNull());

        // Check if a value is present
        if (!value.has_value())
        {
            // No value is present, add a null value
            cJSON_AddItemToObject(root.get(), "value", cJSON_CreateNull());
            return root;
        }

        // Check if the value is a boolean
        if constexpr (is_same<T, bool>::value || is_same<T, bool>::value)
        {
            cJSON_AddItemToObject(root.get(), "value", cJSON_CreateBool(value.value()));
        }

        // Check if the value is a number
        else if constexpr (is_arithmetic<T>::value)
        {
            cJSON_AddItemToObject(root.get(), "value", cJSON_CreateNumber(value.value()));
        }

        // Check if the value is a string
        else if constexpr (is_same<T, string>::value)
        {
            cJSON_AddItemToObject(root.get(), "value", cJSON_CreateString(value.value().c_str()));
        }

        // Check if the value implements ISerializable
        else if constexpr (is_base_of<ISerializable, T>::value)
        {
            cJSON_AddItemToObject(root.get(), "value", cJSON_Parse(value.value().get()->serializeToString().c_str()));
        }

        // Check if the type is a shared pointer to a type that implements ISerializable
        // TODO: We should have some more kind of generic serialization for shared pointers, in fact, this whole function could use a more generic re-write
        else if constexpr (is_same<T, shared_ptr<ISerializable>>::value)
        {
            cJSON_AddItemToObject(root.get(), "value", value.value() == nullptr ? cJSON_CreateNull() : value.value()->serialize().release());
        }

        // The value is not serializable
        else
        {
            ESP_LOGW(RESULT_TAG, "attempted to serialize a non-serializable type %s", typeid(T).name());
        }

        return root;
    }

private:
    Result(bool success, std::unique_ptr<T> value = nullptr, optional<string> error = nullopt)
        : success(success), value(std::move(value)), error(error) {}
};

// Specialization of Result<> when there is no value (void type)
template <>
class Result<void> : ISerializable
{
public:
    bool success;           // Flag indicating success or failure
    optional<string> error; // Optional error message for failure

    /**
     * @brief Create a success result without a value
     *
     * @return Result The result object
     */
    static Result createSuccess()
    {
        return Result(true);
    }

    /**
     * @brief Create a failure result with an error message
     *
     * @param err Error message to store in the result
     * @return Result The result object
     */
    static Result createFailure(const string &err)
    {
        return Result(false, err);
    }

    /**
     * @brief Check if the result is successful
     *
     * @return true If the result is successful
     * @return false If the result is a failure
     */
    bool isSuccess() const
    {
        return success;
    }

    /**
     * @brief Get the error message stored in the result
     *
     * @return string The error message stored in the result
     */
    string getError() const
    {
        if (success || !error)
        {
            ESP_LOGE(RESULT_TAG, "attempted to get error from successful result");
        }
        return *error;
    }

    unique_ptr<cJSON, void (*)(cJSON *item)> serialize() override
    {
        // Create a new cJSON object
        unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_CreateObject(), cJSON_Delete);

        // Serialize the object
        cJSON_AddItemToObject(root.get(), "success", cJSON_CreateBool(success));
        cJSON_AddItemToObject(root.get(), "error", error.has_value() ? cJSON_CreateString(error.value().c_str()) : cJSON_CreateNull());

        return root;
    }

private:
    /**
     * @brief Construct a new Result object
     *
     * @param success Flag indicating success or failure
     * @param error Optional error message to store in the result
     */
    Result(bool success, optional<string> error = nullopt)
        : success(success), error(error) {}
};

#endif // RESULT_H