#include<iostream>
#include "GLFW/glfw3.h"
#include "vulkan/vulkan.hpp"
#include "EngineConfig.h"

using namespace std;

int main() {
    cout << "Engine Version: " << Engine_VERSION_MAJOR << "." 
         << Engine_VERSION_MINOR << endl;
    return 0;
}
