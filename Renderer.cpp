//
// Created by codewing on 20/10/2017.
//
#include "BUILD_OPTIONS.h"

#include "Renderer.h"
#include "Utilities.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <cstring>
#include <set>
#include <algorithm>
#include "Window.h"
#include "QueueFamilyIndices.h"

Renderer::Renderer(int width, int height) {
    window = std::make_shared<Window>(width, height);
    InitVulkan();
}

Renderer::~Renderer() {
    DeInitVulkan();
}

void Renderer::InitVulkan() {
    CreateInstance();
    SetupDebugCallbacks();
    CreateSurface();
    SetupPhysicalDevice();
    InitLogicalDevice();
    CreateSwapchain();
    GetSwapchainImages();
}

void Renderer::DeInitVulkan() {
    DestroySwapchain();
    DeInitLogicalDevice();
    DestroySurface();
    DestroyDebugReportCallbackEXT(instance, callback, nullptr);
    DestroyInstance();
}

void Renderer::CreateInstance() {

#ifdef BUILD_ENABLE_VULKAN_DEBUG
    if(!CheckAllValidationLayersSupported()){
        throw std::runtime_error("more validation layers requested than available");
    }
#endif

    VkApplicationInfo applicationInfo {};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pApplicationName = "Voxel World Generation";
    applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.pEngineName = "No Engine";
    applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceCreateInfo {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;

#ifdef BUILD_ENABLE_VULKAN_DEBUG
    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
#else
    instanceCreateInfo.enabledLayerCount = 0;
#endif

    auto extensions = GetRequiredExtensions();
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

    ErrorCheck(vkCreateInstance(&instanceCreateInfo, nullptr, &instance));
}

void Renderer::DestroyInstance() {
    vkDestroyInstance(instance, nullptr);
}

bool Renderer::CheckAllValidationLayersSupported() {
    // Get all available layers
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayerProperties(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayerProperties.data());

    // check if all required layers are supported
    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayerProperties) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

std::vector<const char *> Renderer::GetRequiredExtensions() {
    std::vector<const char*> extensions;

    unsigned int glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    for (unsigned int i = 0; i < glfwExtensionCount; i++) {
        extensions.push_back(glfwExtensions[i]);
    }

#if BUILD_ENABLE_VULKAN_DEBUG
    extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif

    return extensions;
}

VkBool32 VKAPI_CALL Renderer::debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType,
                                            uint64_t obj, size_t location, int32_t code, const char *layerPrefix,
                                            const char *msg, void *userData) {
    std::cerr << "validation layer: " << msg << std::endl;

    return VK_FALSE;
}

void Renderer::SetupDebugCallbacks() {
#if BUILD_ENABLE_VULKAN_DEBUG
    VkDebugReportCallbackCreateInfoEXT debugReportCallbackCreateInfoEXT {};
    debugReportCallbackCreateInfoEXT.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    debugReportCallbackCreateInfoEXT.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    debugReportCallbackCreateInfoEXT.pfnCallback = debugCallback;

    if (CreateDebugReportCallbackEXT(instance, &debugReportCallbackCreateInfoEXT, nullptr, &callback) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug callback!");
    }
#endif
}

VkResult
Renderer::CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
                                       const VkAllocationCallbacks *pAllocator, VkDebugReportCallbackEXT *pCallback) {
    auto func = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void Renderer::DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback,
                                             const VkAllocationCallbacks *pAllocator) {
    auto func = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
    if (func != nullptr) {
        func(instance, callback, pAllocator);
    }
}

bool Renderer::Run() {
    if(window != nullptr) {
        return window->Update();
    }
    return true;
}

void Renderer::SetupPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (IsDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

bool Renderer::IsDeviceSuitable(VkPhysicalDevice vkPhysicalDevice) {
    QueueFamilyIndices indices = FindQueueFamilies(vkPhysicalDevice);

    bool extensionsSupported = CheckDeviceExtensionSupport(vkPhysicalDevice);

    bool swapChainSupported = false;
    // the swapChain requires extensions
    if(extensionsSupported){
        SwapChainSupportDetails swapChainDetails = QuerySwapchainSupport(vkPhysicalDevice);
        swapChainSupported = !swapChainDetails.formats.empty() && !swapChainDetails.presentModes.empty();
    }

    return indices.isComplete() && swapChainSupported;
}

QueueFamilyIndices Renderer::FindQueueFamilies(VkPhysicalDevice vkPhysicalDevice) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyCount, queueFamilies.data());

    int queueIndex = 0;
    for(const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.SetGraphicsFamily(queueIndex);
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice, queueIndex, surface, &presentSupport);

        if (queueFamily.queueCount > 0 && presentSupport) {
            indices.SetPresentFamily(queueIndex);
        }

        if (indices.isComplete()) {
            break;
        }

        queueIndex++;
    }

    return indices;
}

void Renderer::InitLogicalDevice() {
    QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfoVector;
    std::set<int> uniqueQueueFamilies = {indices.GetGraphicsFamily(), indices.GetPresentFamily()};

    float queuePriority = 1.0f;

    for (int queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfoVector.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures {};

    VkDeviceCreateInfo deviceCreateInfo {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfoVector.data();
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfoVector.size());
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

#if BUILD_ENABLE_VULKAN_DEBUG
    deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
#else
    deviceCreateInfo.enabledLayerCount = 0;
#endif

    ErrorCheck(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device));

    vkGetDeviceQueue(device, indices.GetGraphicsFamily(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.GetPresentFamily(), 0, &presentQueue);
}

void Renderer::DeInitLogicalDevice() {
    vkDestroyDevice(device, nullptr);
}

void Renderer::CreateSurface() {
    if (glfwCreateWindowSurface(instance, window->GetGLFWwindow(), nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

void Renderer::DestroySurface() {
    vkDestroySurfaceKHR(instance, surface, nullptr);
}

void Renderer::CreateSwapchain() {
    SwapChainSupportDetails swapchainDetails = QuerySwapchainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = ChooseSwapchainSurfaceFormat(swapchainDetails.formats);
    VkPresentModeKHR presentMode = ChooseSwapchainPresentMode(swapchainDetails.presentModes);
    VkExtent2D extent = ChooseSwapchainExtent(swapchainDetails.capabilities);

    // number of swap chain images
    uint32_t imageCount = swapchainDetails.capabilities.minImageCount + 1;
    if (swapchainDetails.capabilities.maxImageCount > 0 && imageCount > swapchainDetails.capabilities.maxImageCount) {
        imageCount = swapchainDetails.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchainCreateInfo {};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = surface;
    swapchainCreateInfo.minImageCount = imageCount;
    swapchainCreateInfo.imageFormat = surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent = extent;
    swapchainCreateInfo.imageArrayLayers = 1;
    // render directly to image - VK_IMAGE_USAGE_TRANSFER_DST_BIT for post processing or other operations before rendering
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = {(uint32_t) indices.GetGraphicsFamily(), (uint32_t) indices.GetPresentFamily()};

    if (indices.GetGraphicsFamily() != indices.GetPresentFamily()) {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0; // Optional
        swapchainCreateInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    swapchainCreateInfo.preTransform = swapchainDetails.capabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Don't blend with other windows
    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE; // Used for swapchain re-creation

    ErrorCheck(vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain));

    // save the format and the extent
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void Renderer::DestroySwapchain() {
    vkDestroySwapchainKHR(device, swapchain, nullptr);
}

void Renderer::GetSwapchainImages() {
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());
}

/// check whether the required extensions are present
bool Renderer::CheckDeviceExtensionSupport(VkPhysicalDevice vkPhysicalDevice) {

    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

/// Get the swapchain info
SwapChainSupportDetails Renderer::QuerySwapchainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if(formatCount != 0){
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if(presentModeCount != 0){
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

/// Color format of the surface
VkSurfaceFormatKHR Renderer::ChooseSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
    // No preference?
    if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
        return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }

    // Our preference
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    // Otherwise the first should be fine
    return availableFormats[0];
}

/// In which order will the image be presented? Fifo -> Add image to queue | Mailbox Triple Buffer with replace | Immediate
VkPresentModeKHR Renderer::ChooseSwapchainPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) {
    VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        } else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            bestMode = availablePresentMode;
        }
    }

    return bestMode;
}

/// Size of the rendered image
VkExtent2D Renderer::ChooseSwapchainExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = {static_cast<uint32_t>(window->GetWidth()),
                                   static_cast<uint32_t>(window->GetHeight())};

        actualExtent.width = std::max(capabilities.minImageExtent.width,
                                      std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height,
                                       std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}



