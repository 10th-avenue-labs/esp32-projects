#include <iostream>
#include <string>

class Led {
private:
    int width;
    int height;

public:
    // Constructor
    Rectangle(int w, int h) : width(w), height(h) {}

    // Methods
    int area() {
        return width * height;
    }

    void printDimensions() {
        std::cout << "Width: " << width << ", Height: " << height << std::endl;
    }
};


extern "C" void app_main(){
    Rectangle rect(5, 10);

    std::cout << "Area: " << rect.area() << std::endl;
    rect.printDimensions();

}