#pragma once

#include <vulkan/vulkan.h>
#include "XSArray.h"
#include "XSShader.h"

class XSDisplay 
{
    VkInstance                 _vkInstance;
    VkPhysicalDevice           _phyDev = VK_NULL_HANDLE;
    VkPhysicalDeviceFeatures   _phyDevFeatures{};
    uint32_t                   _graphicsFamily = -1;
    VkDevice                   _vkDev;
    VkQueue                    _graphicsQueue;
    VkSurfaceKHR               _vkSurface;
    VkDisplayKHR               _display = VK_NULL_HANDLE;
	VkDisplayModeKHR           _displayMode = VK_NULL_HANDLE;
	VkDisplayModePropertiesKHR _displayModeProperties;
    VkSwapchainKHR             _vkSwapChain;
    VkExtent2D                 _vkSwapChainExtent;
    VkFormat                   _vkSwapChainImageFormat;
    XSArray<VkImage>           _vkSwapChainImages;
    XSArray<VkImageView>       _vkSwapChainImageViews;
    VkRenderPass               _vkRenderPass;
    VkPipelineLayout           _vkPipelineLayout;
    VkPipeline                 _vkGraphicsPipeline;
    XSArray<VkFramebuffer>     _vkSwapChainFramebuffers;
    VkCommandPool              _vkCommandPool;
    VkCommandBuffer            _vkCommandBuffer;
    VkSemaphore                _imageAvailableSemaphore;
    VkSemaphore                _renderFinishedSemaphore;
    VkFence                    _inFlightFence;

    void _initVulkan();
    void _createInstance();
    void _pickPhyDevice();
    void _createSurface();
    void _createXSRDev();
    void _createSwapChain();
    void _createImageViews();
    void _destroyImageViews();
    void _createRenderPass();
    void _createGraphicsPipeline(); //TODO: Different shaders
    void _createFramebuffers();
    void _destroyFramebuffers();
    void _createCommandPool();
    void _createCommandBuffer();
    void _createSyncObjects();
    void _cleanUp();

    public:
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t rate = 0;

    XSDisplay();
    void drawFrame();
    void recordCommandBuffer( VkCommandBuffer commandBuffer, uint32_t imageIndex );
    ~XSDisplay();
};
