#include <iostream>
#include <variant>  // For std::monostate

// Use std::monostate as a default type for the "no value" scenario
template <typename T = std::monostate>  // Default type is `std::monostate`
class MyClass {
public:
    T value;

    // Constructor for non-std::monostate types
    MyClass(T val) : value(val) {}

    // Constructor for std::monostate (no value)
    MyClass() {
        if constexpr (std::is_same_v<T, std::monostate>) {
            std::cout << "MyClass instantiated without a type parameter (no value)." << std::endl;
        }
    }

    void print() const {
        if constexpr (std::is_same_v<T, std::monostate>) {
            std::cout << "MyClass has no value." << std::endl;
        } else {
            std::cout << "MyClass has value: " << value << std::endl;
        }
    }
};

int main() {
    // Case 1: MyClass with an int type parameter
    MyClass<int> obj1(42);
    obj1.print();  // Output: MyClass has value: 42

    // Case 2: MyClass with no type parameter (defaults to std::monostate)
    MyClass obj2;  // Uses default std::monostate type
    obj2.print();  // Output: MyClass has no value.

    return 0;
}
