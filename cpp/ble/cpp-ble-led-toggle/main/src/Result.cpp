#include "Result.h"

using namespace std;

template<typename T>
class Result {

    private:
        bool success;
        string failureReason;
        T value;

        Result(bool success, string failureReason, T value):
            success(success),
            failureReason(failureReason),
            value(value)
        {
        };
    public:
        static Result failure(string reason) {
            return Result(false, reason, NULL);
        }

        static Result success(T& value) {
            return Result(true, NULL, value);
        }

        bool isSuccess() const {
            return success;
        }

        string getFailureReason() const {
            return success ? "" : failureReason;
        }

        T getValue() const {
            if (!success) {
                throw runtime_error("Attempted to access value from a failed result.");
            }
            return value;
        }
};