#ifndef RESULT_H
#define RESULT_H

#include <optional>
#include <string>
#include <type_traits>
#include <esp_log.h>

using namespace std;

static const char* RESULT_TAG = "RESULT";

// Primary template for Result with a value of type T
template <typename T = void>
class Result {
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
        static Result createSuccess(T value) {
            return Result(true, value);
        }

        /**
         * @brief Create a failure result with an error message
         * 
         * @param err Error message to store in the result
         * @return Result The result object
         */
        static Result createFailure(const string& err) {
            return Result(false, nullopt, err);
        }

        /**
         * @brief Check if the result is successful
         * 
         * @return true If the result is successful
         * @return false If the result is a failure
         */
        bool isSuccess() const {
            return success;
        }

        /**
         * @brief Get the value stored in the result
         * 
         * @return T The value stored in the result
         */
        T getValue() const {
            if (!success || !value) {
                ESP_LOGE(RESULT_TAG, "attempted to get value from failed result");
            }
            return *value;
        }

        /**
         * @brief Get the error message stored in the result
         * 
         * @return string The error message stored in the result
         */
        string getError() const {
            if (success || !error) {
                ESP_LOGE(RESULT_TAG, "attempted to get error from successful result");
            }
            return *error;
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

// Specialization of Result<> when there is no value (void type)
template <>
class Result<void> {
    public:
        bool success;           // Flag indicating success or failure
        optional<string> error; // Optional error message for failure

        /**
         * @brief Create a success result without a value
         * 
         * @return Result The result object
         */
        static Result createSuccess() {
            return Result(true);
        }

        /**
         * @brief Create a failure result with an error message
         * 
         * @param err Error message to store in the result
         * @return Result The result object
         */
        static Result createFailure(const string& err) {
            return Result(false, err);
        }

        /**
         * @brief Check if the result is successful
         * 
         * @return true If the result is successful
         * @return false If the result is a failure
         */
        bool isSuccess() const {
            return success;
        }

        /**
         * @brief Get the error message stored in the result
         * 
         * @return string The error message stored in the result
         */
        string getError() const {
            if (success || !error) {
                ESP_LOGE(RESULT_TAG, "attempted to get error from successful result");
            }
            return *error;
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