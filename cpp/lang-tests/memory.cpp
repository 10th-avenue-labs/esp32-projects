// Online C++ compiler to run C++ program online
#include <iostream>

using namespace std;

class Test {
    public:
        string name;
        int id;

        // Regular constructor. Take a copy of a string name
        Test(string name) {
            this->name = name;
            this->id = rand();
            printf("Creating '%s' with id '%d' at addr '%p'\n", name.c_str(), id, &id);
        }
        
        // Copy constructor. Created when the obj is copied, like when passing to another function as a variable
        // This will copy the name but not the id
        Test(const Test& other) {
            this->name = other.name;
            this->id = rand();
            printf(
"Copying: org, cpy\n \
    Name: %s, %s\n \
    Id  : %d, %d\n \
    Addr: %p, %p\n",
    other.name.c_str(), name.c_str(),
    other.id, id,
    &other.id, &id
);
        }
        
        // Move constructor
        // &&other is an rvalue reference, meaning it's a temporary variable that will soon be destroyed. All of it's data can sort of be "stolen" or "moved"
        Test(Test&& other): name(move(other.name)) {
            this->id = rand();
            
            // The name will no longer actually point to anything for the old 'org' value
            // The ID will still be valid in the 'org' object, because we did not move it
            
            printf(
"Moving: org, mvd\n \
    Name: %s, %s\n \
    Id  : %d, %d\n \
    Addr: %p, %p\n",
                other.name.c_str(), name.c_str(),
                other.id, id,
                &other.id, &id
            ); 
        }
        
        // Assignment operator. Kinda like copy, but happens when you do something like `a = b`
        Test& operator=(Test& other) {
            this->name = other.name;
            this-> id = rand();
            printf(
"Assigning: org, cpy\n \
    Name: %s, %s\n \
    Id  : %d, %d\n \
    Addr: %p, %p\n",
    other.name.c_str(), name.c_str(),
    other.id, id,
    &other.id, &id
); 
            return *this;
        }
        
        // Destructor. Called when the object is destroyed
        ~Test() {
            printf("Destroying '%s' with id '%d' at addr '%p'\n", name.c_str(), id, &id);
        }
};

void func(Test test) {
    // printf("test in func addr %p\n", test.getIdAddr());
}

class Carrier {
    public:
        Test myTest;
        Carrier(Test t): myTest(t) {
            
        }
};

class EfficientCarrier {
    public:
        Test myTest;
        EfficientCarrier(Test t) : myTest(move(t)) {
            
        }
};

int main() {
    srand(time(0));
    Test myTest("An object"); // Constructor. This will call the constructor and create a new object
    func(myTest); // This function passes by value, it will call the copy constructor creating a new object. Once the function completes, the destructor will be called
    Carrier carrier(myTest); // This function passes by value, first to the constructor, and then again by value to the initializer list. This end up making 2 copies, 1 of which will be immidiately destroyed
    
    EfficientCarrier efficient(myTest); // This function passes by value, first to the constructor, but then moves the copied value to the objects member. This utilizes the move constructor on the Test object and is slightly more efficient.
    
    printf("before return\n");

    return 0;
}
