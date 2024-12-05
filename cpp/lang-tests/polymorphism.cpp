#include <iostream>
#include <vector>

using namespace std;

class ICarPart;

class Car {
    public:
        vector<ICarPart*> parts;
};

class ICarPart {
    public:
        virtual void print() = 0;
        virtual ~ICarPart() {}
};

class Engine : public ICarPart {
    public:
        void print() {
            cout << "Engine" << endl;
        }

        void start() {
            cout << "Engine started" << endl;
        }
};

class Wheel : public ICarPart {
    public:
        void print() {
            cout << "Wheel" << endl;
        }
};


int main() {
    

    // Create a car
    Car car;

    // Add parts to the car
    car.parts.push_back(new Engine());
    car.parts.push_back(new Wheel());
    car.parts.push_back(new Wheel());

    for (auto& part : car.parts) {
        part->print();

        // Check if the part is an engine
        Engine* engine = dynamic_cast<Engine*>(part);
        if (engine) {
            engine->start();
        }
    }

    return 0;
}