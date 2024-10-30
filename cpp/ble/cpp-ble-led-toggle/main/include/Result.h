#ifndef RESULT_H
#define RESULT_H

#include <string>
#include <stdexcept>

template<typename T>
class Result {
private:
    bool success;
    std::string failureReason;
    T value;

    Result(bool success, const std::string& failureReason, T value);

public:
    static Result<T> failure(const std::string& reason);
    static Result<T> success(const T& value);
    bool isSuccess() const;
    std::string getFailureReason() const;
    T getValue() const;
};

#endif