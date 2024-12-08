#include <iostream>
#include <optional>
#include <string>
#include <type_traits>

// Primary template for Result with a value of type T
template <typename T = void>
class Result {
public:
    bool success;  // Flag indicating success or failure
    std::optional<T> value;  // Optional value for success
    std::optional<std::string> error;  // Optional error message for failure

    // Private constructor to ensure controlled creation
private:
    // Success constructor with or without a value
    Result(bool success, std::optional<T> value = std::nullopt, std::optional<std::string> error = std::nullopt)
        : success(success), value(value), error(error) {}

public:
    // Static method for creating a success with a value
    static Result createSuccess(T val) {
        return Result(true, val);
    }

    // Static method for creating a failure with an error message
    static Result createFailure(const std::string& err) {
        return Result(false, std::nullopt, err);
    }

    // Method to check if the result is successful
    bool isSuccess() const {
        return success;
    }

    // Method to get the value (if success)
    T getValue() const {
        if (success && value) {
            return *value;
        }
        throw std::runtime_error("Attempt to access value of a failed result.");
    }

    // Method to get the error message (if failure)
    std::string getError() const {
        if (!success && error) {
            return *error;
        }
        throw std::runtime_error("Attempt to access error of a successful result.");
    }
};

// Specialization of Result<> when there is no value (void type)
template <>
class Result<void> {
public:
    bool success;  // Flag indicating success or failure
    std::optional<std::string> error;  // Optional error message for failure

    // Private constructor to ensure controlled creation
private:
    // Success constructor without a value
    Result(bool success, std::optional<std::string> error = std::nullopt)
        : success(success), error(error) {}

public:
    // Static method for creating a success without a value
    static Result createSuccess() {
        return Result(true);
    }

    // Static method for creating a failure with an error message
    static Result createFailure(const std::string& err) {
        return Result(false, err);
    }

    // Method to check if the result is successful
    bool isSuccess() const {
        return success;
    }

    // Method to get the error message (if failure)
    std::string getError() const {
        if (!success && error) {
            return *error;
        }
        throw std::runtime_error("Attempt to access error of a successful result.");
    }
};

class MyObj {
public:
    int value;

    MyObj(int val) : value(val) {}
};

// Example usage of the Result class
int main() {
    // Success without a value
    Result successWithoutValue = Result<>::createSuccess();
    std::cout << "Success without value: " << successWithoutValue.isSuccess() << std::endl;

    // Success with a value
    Result<int> successWithValue = Result<int>::createSuccess(42);
    std::cout << "Success with value: " << successWithValue.isSuccess() << ", Value: " << successWithValue.getValue() << std::endl;

    // Success with an object value
    MyObj obj(100);
    Result<MyObj> successWithObject = Result<MyObj>::createSuccess(obj);
    std::cout << "Success with object value: " << successWithObject.isSuccess() << ", Value: " << successWithObject.getValue().value << std::endl;

    // Failure
    Result failure = Result<>::createFailure("An error occurred");
    std::cout << "Failure: " << failure.isSuccess() << ", Error: " << failure.getError() << std::endl;

    return 0;
}
