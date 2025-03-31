#include <iostream>
#include <vulkan/vulkan_core.h>

int main() {
    uint32_t d;
    vkEnumerateInstanceLayerProperties(&d, nullptr);
    std::cout << "test!" << d << std::endl;
    return 0;
}