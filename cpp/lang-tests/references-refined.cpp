#include <iostream>
#include <memory>
#include <vector>

using namespace std;

class Characteristic {
    public:
        string name;
        int id;
        string creationMethod;

        // This create a new characteristic from scratch
        Characteristic(string name) : name(name), creationMethod("Constructor"), id(rand()) {
            printf("Constructing id: %d\n", id);

            // TODO: What happens when we create a characteristic?
        }

        // This copies a characteristic from another characteristic
        // At the end of this, both this and the other characteristic will both still be valid and will both have unique IDs
        Characteristic(const Characteristic& other) : name(other.name), creationMethod("Copy Constructor"), id(rand()) {
            printf("Copying.\t\tThis\t\t\tOther\t\t\tSame\n");
            printf("\tName:\t\t'%s',\t\t'%s',\t\t%d\n", name.c_str(), other.name.c_str(), name == other.name);  // The name will be the same because it is copied
            printf("\tName addr:\t'%p',\t'%p',\t%d\n", &name, &other.name, &name == &other.name);               // The address of the name will be different because it is a new object
            printf("\tId:\t\t'%d',\t\t'%d',\t\t%d\n", id, other.id, id == other.id);                            // The ID will be different because it is a new object
            printf("\tId addr:\t'%p',\t'%p',\t%d\n", &id, &other.id, &id == &other.id);                         // The address of the ID will be different because it is a new object

            // TODO: What happens when we copy a characteristic?
        }

        // This (sort of) creates a new characteristic from another characteristic
        // We can safely move all the other characteristics data out of the other characteristic and into this one because it will soon be destroyed anyway
        Characteristic(Characteristic&& other) : name(move(other.name)), id(move(other.id)), creationMethod("Move Constructor") {
            printf("Moving.\t\t\tThis\t\t\tOther\t\t\tSame\n");
            printf("\tName:\t\t'%s',\t\t'%s',\t\t\t%d\n", name.c_str(), other.name.c_str(), name == other.name);    // The name will be different because it is moved. The original object will no longer have a name
            printf("\tName addr:\t'%p',\t'%p',\t%d\n", &name, &other.name, &name == &other.name);                   // The address of the name will be different because, even though we're moving all the objects data, it is in fact being moved to a new location
            printf("\tId:\t\t'%d',\t\t'%d',\t\t%d\n", id, other.id, id == other.id);                                // The ID will still be the same because, for primitive types, moving is the same as copying
            printf("\tId addr:\t'%p',\t'%p',\t%d\n", &id, &other.id, &id == &other.id);                             // The address of the ID will be different because it is still a new object, even though we've 'stolen' all the data we can

            // TODO: What happens when we move a characteristic?
        }

        ~Characteristic() {
            printf("Destructing %s\n", creationMethod.c_str());

            // TODO: What happens when we destroy a characteristic?
        }

        void notify() {
            printf("Notifying %s\n", name.c_str());
        }
};

class Service {
    public:
        vector<shared_ptr<Characteristic>> characteristics;

        // Constructor for sole ownership
        Service(vector<Characteristic>&& characteristics) {
            for (auto& characteristic : characteristics) {
                // Convert each Characteristic to a shared_ptr and move it
                this->characteristics.emplace_back(make_shared<Characteristic>(move(characteristic)));
            }
            printf("Created service with sole ownership over %ld characteristics\n", this->characteristics.size());
        }

        // Constructor for shared ownership
        Service(vector<shared_ptr<Characteristic>> characteristics) : characteristics(characteristics) {
            printf("Created service with shared ownership over %ld characteristics\n", this->characteristics.size());
        }
};


int main() {
    srand(time(0));

    // Create a service with sole ownership over characteristics
    // Note: The vector prefers to use copy constructors over move constructors
    Service s1({Characteristic("thing")});

    printf("Making shared\n\n");

    // Create a service with shared ownership over characteristics
    shared_ptr<Characteristic> c1 = make_shared<Characteristic>("thing");
    Service s2({c1});

    // // Use case 1

    // // Create characteristic
    // Characteristic mtu("MTU");
    // Characteristic sendData("Send Data"); // From the perspective of the Central, so on this device it actually is receiving data
    // Characteristic receiveData("Receive Data"); // From the perspective of the Central, so on this device it actually is sending data

    // // Create the service
    // Service mtuService({mtu, sendData, receiveData}); // This needs to have the same exact handles as the characteristic used below

    // // Send a notification on the receive data characteristic
    // receiveData.notify(); // This needs to have the same exact handles as the characteristic in the service



    // // Use case 2

    // // Create service

}