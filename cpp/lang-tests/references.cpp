#include <iostream>

using namespace std;

class Characteristic {
    public:
        string name;
        int id;
        string creationMethod;

        // This create a new characteristic from scratch
        Characteristic(string name) : name(name), creationMethod("Constructor"), id(rand()) {
            printf("Constructing id: %d\n", id);
        }

        // This copies a characteristic from another characteristic
        // At the end of this, both this and the other characteristic will both still be valid and will both have unique IDs
        Characteristic(Characteristic& other) : name(other.name), creationMethod("Copy Constructor"), id(rand()) {
            printf("Copying.\t\tThis\t\t\tOther\t\t\tSame\n");
            printf("\tName:\t\t'%s',\t\t'%s',\t\t%d\n", name.c_str(), other.name.c_str(), name == other.name);  // The name will be the same because it is copied
            printf("\tName addr:\t'%p',\t'%p',\t%d\n", &name, &other.name, &name == &other.name);               // The address of the name will be different because it is a new object
            printf("\tId:\t\t'%d',\t\t'%d',\t\t%d\n", id, other.id, id == other.id);                            // The ID will be different because it is a new object
            printf("\tId addr:\t'%p',\t'%p',\t%d\n", &id, &other.id, &id == &other.id);                         // The address of the ID will be different because it is a new object
        }

        // This (sort of) creates a new characteristic from another characteristic
        // We can safely move all the other characteristics data out of the other characteristic and into this one because it will soon be destroyed anyway
        Characteristic(Characteristic&& other) : name(move(other.name)), id(move(other.id)), creationMethod("Move Constructor") {
            printf("Moving.\t\t\tThis\t\t\tOther\t\t\tSame\n");
            printf("\tName:\t\t'%s',\t\t'%s',\t\t\t%d\n", name.c_str(), other.name.c_str(), name == other.name);    // The name will be different because it is moved. The original object will no longer have a name
            printf("\tName addr:\t'%p',\t'%p',\t%d\n", &name, &other.name, &name == &other.name);                   // The address of the name will be different because, even though we're moving all the objects data, it is in fact being moved to a new location
            printf("\tId:\t\t'%d',\t\t'%d',\t\t%d\n", id, other.id, id == other.id);                                // The ID will still be the same because, for primitive types, moving is the same as copying
            printf("\tId addr:\t'%p',\t'%p',\t%d\n", &id, &other.id, &id == &other.id);                             // The address of the ID will be different because it is still a new object, even though we've 'stolen' all the data we can
        }

        ~Characteristic() {
            printf("Destructing %s\n", creationMethod.c_str());
        }

        void notify() {
            printf("Notifying %s\n", name.c_str());
        }
};

class Service {
    public:
        Characteristic c;

        // Create a service from a characteristic passed by value. This will create a copy of the characteristic
        Service(Characteristic c) : c(move(c)) {
            // This will do a construct, copy, move for Characteristic c("name"); Service s(c);
            // This will do a construct, move for Service s(Characteristic("name"));
            printf("Service created\n");
        }

};


int main() {
    srand(time(0));
    // construct, copy, move
    Characteristic c1("Strength");
    Service s1(c1);
    printf("Break\n");
    // Construct move
    Service s2(Characteristic("Dexterity"));
    return 0;
}