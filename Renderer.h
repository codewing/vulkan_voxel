//
// Created by codewing on 20/10/2017.
//

#ifndef VULKAN_VOXEL_RENDERER_H
#define VULKAN_VOXEL_RENDERER_H

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

class Window;
class QueueFamilyIndices;

class Renderer {

private:

    // Variables
    Window* window;

    VkInstance vkInstance = VK_NULL_HANDLE;
    VkPhysicalDevice vkPhysicalDevice = VK_NULL_HANDLE;
    VkDevice vkDevice = VK_NULL_HANDLE;

    VkQueue graphicsQueue = VK_NULL_HANDLE;


    VkDebugReportCallbackEXT callback = VK_NULL_HANDLE;

    const std::vector<const char*> validationLayers = {"VK_LAYER_LUNARG_standard_validation"};

    // Methods
    void InitVulkan();
    void DeInitVulkan();

    void CreateInstance();
    void DeleteInstance();

    void SetupPhysicalDevice();

    void InitLogicalDevice();
    void DeInitLogicalDevice();

    bool CheckAllValidationLayersSupported();
    std::vector<const char*> GetRequiredExtensions();

    // Debug callbacks
    void SetupDebugCallbacks();
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData);
    VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback);
    void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator);

    bool IsDeviceSuitable(VkPhysicalDevice vkPhysicalDevice);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice vkPhysicalDevice);

public:

    Renderer(int width, int height);
    ~Renderer();

    bool Run();

};


#endif //VULKAN_VOXEL_RENDERER_H
