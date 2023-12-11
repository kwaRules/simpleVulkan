#include "XSDisplay.h"
#include <iostream>
#include <cstring>

XSDisplay::XSDisplay()
{
    _initVulkan();
}

void 
XSDisplay::_createInstance() 
{
    VkApplicationInfo XSInfo{};
    XSInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    XSInfo.pApplicationName = "XSapphire";
    XSInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
    XSInfo.pEngineName = "No Engine";
    XSInfo.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );
    XSInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo XSRCreateInfo{};
    XSRCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    XSRCreateInfo.pApplicationInfo = &XSInfo;

    //TODO: Wrap in func?
    uint32_t XSRExtCount = 2;
    const char *XSRExts[] = { "VK_KHR_surface", "VK_KHR_display" };
    const char *validLayer[] = { "VK_LAYER_KHRONOS_validation" };
    XSRCreateInfo.enabledExtensionCount = XSRExtCount;
    XSRCreateInfo.ppEnabledExtensionNames = XSRExts;
    XSRCreateInfo.enabledLayerCount = 1;
    XSRCreateInfo.ppEnabledLayerNames = validLayer;
    
    if( vkCreateInstance( &XSRCreateInfo, nullptr, &_vkInstance ) != VK_SUCCESS ) 
    {
        printf( "Failed to create vulkan instance\n" );
        exit( 1 );
    }
}

void
XSDisplay::_pickPhyDevice() 
{
    uint32_t phyDevCount = 0;
    uint32_t queueFamilyCount = 0;

    vkEnumeratePhysicalDevices( _vkInstance, &phyDevCount, nullptr );
    if( phyDevCount == 0 ) 
    {
        printf( "Failed to find a vulkan supported device\n" );
        exit( 1 );
    }

    VkPhysicalDevice XSRPhyDev[phyDevCount];
    vkEnumeratePhysicalDevices( _vkInstance, &phyDevCount, XSRPhyDev );
    //TODO: Wrap in func
    for( int i=0; i<phyDevCount; i++ ) 
    {
        //findQueueFamily (graphics for now)
        vkGetPhysicalDeviceQueueFamilyProperties( XSRPhyDev[i], &queueFamilyCount, nullptr );
        VkQueueFamilyProperties queueFamilies[queueFamilyCount];
        vkGetPhysicalDeviceQueueFamilyProperties( XSRPhyDev[i], &queueFamilyCount, queueFamilies );
        for( int k=0; k<queueFamilyCount; k++ ) 
        {
            if( queueFamilies[k].queueFlags & VK_QUEUE_GRAPHICS_BIT ) 
            {
                _graphicsFamily = k;
                break;
            }
        }

        if( _graphicsFamily == -1 ) 
        {
            printf( "Failed to find a graphics queue for vulkan device\n" );
            exit( 1 );
        }

        //isDeviceSuitable (discrete gpu for now)
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties( XSRPhyDev[i], &deviceProperties );
        if( deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ) 
        {
            //TODO: WRAP IN FUNC
            uint32_t extensionCount;
            vkEnumerateDeviceExtensionProperties( XSRPhyDev[i], nullptr, &extensionCount, nullptr );
            VkExtensionProperties availableExtensions[extensionCount];
            vkEnumerateDeviceExtensionProperties( XSRPhyDev[i], nullptr, &extensionCount, availableExtensions );

            //TODO: CHECK EXTS IN A BETTER WAY
            for( int k=0; k<extensionCount; k++ ) 
            {
                if( strcmp( availableExtensions[k].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME ) == 0 ) 
                {

                    _phyDev = XSRPhyDev[i];
                    break;
                }
            }
        }
    }

    if( _phyDev == VK_NULL_HANDLE ) 
    {
        printf( "Failed to find a suitable vulkan device\n" );
        exit( 1 );
    }
}

void
XSDisplay::_createSurface() 
{
    uint32_t displayCount;
	vkGetPhysicalDeviceDisplayPropertiesKHR( _phyDev, &displayCount, NULL );
	VkDisplayPropertiesKHR pDisplayProperties[displayCount];
	vkGetPhysicalDeviceDisplayPropertiesKHR( _phyDev, &displayCount, pDisplayProperties );

    uint32_t planeCount;
	vkGetPhysicalDeviceDisplayPlanePropertiesKHR( _phyDev, &planeCount, NULL );
	VkDisplayPlanePropertiesKHR pPlaneProperties[planeCount];
	vkGetPhysicalDeviceDisplayPlanePropertiesKHR( _phyDev, &planeCount, pPlaneProperties );

    for( uint32_t i=0; i<displayCount; i++ ) 
    {
        _display = pDisplayProperties[i].display;

		uint32_t modeCount;
		vkGetDisplayModePropertiesKHR( _phyDev, _display, &modeCount, NULL );
		VkDisplayModePropertiesKHR pModeProperties[modeCount];
		vkGetDisplayModePropertiesKHR( _phyDev, _display, &modeCount, pModeProperties );

        //choose the highest resolution and refresh rate
        for( int k=0; k<modeCount; k++ ) 
        {
            const VkDisplayModePropertiesKHR *mode = &pModeProperties[k];
			if ( mode->parameters.visibleRegion.width >= width && mode->parameters.visibleRegion.height >= height && mode->parameters.refreshRate >= rate )
			{
				_displayMode = mode->displayMode;
                _displayModeProperties = pModeProperties[k];
                width = mode->parameters.visibleRegion.width;
                height = mode->parameters.visibleRegion.height;
                rate = mode->parameters.refreshRate;
			}
        }
        //TODO: implement multiple displays
        break;
    }

    if( _displayMode == VK_NULL_HANDLE or _display == VK_NULL_HANDLE ) 
    {
        printf( "Failed to create a display surface\n" );
        exit( 1 );
    }

    uint32_t bestPlaneIndex = UINT32_MAX;
    for( int i=0; i<planeCount; i++ ) 
    {
        uint32_t planeDisplayCount;
        vkGetDisplayPlaneSupportedDisplaysKHR( _phyDev, i, &planeDisplayCount, NULL );
        VkDisplayKHR pDisplays[planeDisplayCount];
		vkGetDisplayPlaneSupportedDisplaysKHR( _phyDev, i, &planeDisplayCount, pDisplays );

        bestPlaneIndex = UINT32_MAX;
        for( int k=0; k<planeDisplayCount; k++ ) 
        {
            if( _display == pDisplays[k] ) 
            {
                bestPlaneIndex = i;
				break;
            }
        }

        if( bestPlaneIndex != UINT32_MAX ) break;
    }

    if( bestPlaneIndex == UINT32_MAX ) 
    {
        printf( "Failed to find a display plane\n" );
        exit( 1 );
    }

    VkDisplayPlaneCapabilitiesKHR planeCap;
	vkGetDisplayPlaneCapabilitiesKHR( _phyDev, _displayMode, bestPlaneIndex, &planeCap );
	VkDisplayPlaneAlphaFlagBitsKHR alphaMode = (VkDisplayPlaneAlphaFlagBitsKHR)0;

    if( planeCap.supportedAlpha & VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_PREMULTIPLIED_BIT_KHR )
	{
		alphaMode = VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_PREMULTIPLIED_BIT_KHR;
	}
	else if( planeCap.supportedAlpha & VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_BIT_KHR )
	{
		alphaMode = VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_BIT_KHR;
	}
	else if( planeCap.supportedAlpha & VK_DISPLAY_PLANE_ALPHA_GLOBAL_BIT_KHR )
	{
		alphaMode = VK_DISPLAY_PLANE_ALPHA_GLOBAL_BIT_KHR;
	}
	else if( planeCap.supportedAlpha & VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR )
	{
		alphaMode = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR;
	}

    VkDisplaySurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.displayMode = _displayMode;
	createInfo.planeIndex = bestPlaneIndex;
	createInfo.planeStackIndex = pPlaneProperties[bestPlaneIndex].currentStackIndex;
	createInfo.transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	createInfo.globalAlpha = 1.0;
	createInfo.alphaMode = alphaMode;
	createInfo.imageExtent.width = width;
	createInfo.imageExtent.height = height;

	if( vkCreateDisplayPlaneSurfaceKHR( _vkInstance, &createInfo, NULL, &_vkSurface ) != VK_SUCCESS ) 
    {
		printf( "Failed to create display plane surface\n" );
        exit( 1 );
	}

    // TODO: IMPLEMENT vkGetPhysicalDeviceSurfaceSupportKHR()
}

void
XSDisplay::_createXSRDev() 
{
    float queuePriority = 1.0f;
    const char *devExts[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = _graphicsFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &_phyDevFeatures;
    createInfo.enabledLayerCount = 0;
    createInfo.enabledExtensionCount = 1;
    createInfo.ppEnabledExtensionNames = devExts;

    if ( vkCreateDevice( _phyDev, &createInfo, nullptr, &_vkDev ) != VK_SUCCESS ) 
    {
        printf( "Failed to create a vulkan device\n" );
        exit( 1 );
    }

    vkGetDeviceQueue( _vkDev, _graphicsFamily, 0, &_graphicsQueue );
}

void 
XSDisplay::_createSwapChain() 
{
    VkSurfaceCapabilitiesKHR capabilities;
    XSArray<VkSurfaceFormatKHR> formats;
    XSArray<VkPresentModeKHR> presentModes;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR( _phyDev, _vkSurface, &capabilities );

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR( _phyDev, _vkSurface, &formatCount, nullptr );
    if( formatCount != 0 ) 
    {
        formats.resize( formatCount );
        vkGetPhysicalDeviceSurfaceFormatsKHR( _phyDev, _vkSurface, &formatCount, formats.data() );
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR( _phyDev, _vkSurface, &presentModeCount, nullptr );
    if( presentModeCount != 0 ) 
    {
        presentModes.resize( presentModeCount );
        vkGetPhysicalDeviceSurfacePresentModesKHR( _phyDev, _vkSurface, &presentModeCount, presentModes.data() );
    }

    if( formats.empty() or presentModes.empty() ) 
    {
        printf( "Failed to find supported formats or presentModes\n" );
        exit( 1 );
    }

    VkSurfaceFormatKHR t_format = formats[0];
    VkPresentModeKHR t_presentMode = VK_PRESENT_MODE_FIFO_KHR;
    VkExtent2D swapExtent = { width, height };
    uint32_t imageCount = capabilities.minImageCount;

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = _vkSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = t_format.format;
    createInfo.imageColorSpace = t_format.colorSpace;
    createInfo.imageExtent = swapExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = t_presentMode;
    createInfo.clipped = VK_FALSE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if ( vkCreateSwapchainKHR( _vkDev, &createInfo, nullptr, &_vkSwapChain ) != VK_SUCCESS ) 
    {
        printf( "Failed to create swapchain\n" );
        exit( 1 );
    }

    vkGetSwapchainImagesKHR( _vkDev, _vkSwapChain, &imageCount, nullptr );
    _vkSwapChainImages.resize( imageCount );
    vkGetSwapchainImagesKHR( _vkDev, _vkSwapChain, &imageCount, _vkSwapChainImages.data() );

    _vkSwapChainExtent = swapExtent;
    _vkSwapChainImageFormat = t_format.format;
}

void 
XSDisplay::_createImageViews() 
{
    _vkSwapChainImageViews.resize( _vkSwapChainImages.size() );
    printf( "Swapchain Images size: %d\n", _vkSwapChainImages.size() );

    for( int i=0; i<_vkSwapChainImages.size(); i++ ) 
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = _vkSwapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = _vkSwapChainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        
        if ( vkCreateImageView( _vkDev, &createInfo, nullptr, &_vkSwapChainImageViews[i]) != VK_SUCCESS ) 
        {
            printf( "Failed to create imageViews\n" );
            exit( 1 );
        }
    }
}

void 
XSDisplay::_destroyImageViews() 
{
    for( const auto &view : _vkSwapChainImageViews ) 
    {
        vkDestroyImageView( _vkDev, view, nullptr );
    }
}

void 
XSDisplay::_createRenderPass() 
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = _vkSwapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if ( vkCreateRenderPass( _vkDev, &renderPassInfo, nullptr, &_vkRenderPass ) != VK_SUCCESS ) 
    {
        printf( "Failed to create Render Pass\n" );
        exit( 1 );
    }
}

void 
XSDisplay::_createGraphicsPipeline() 
{
    XSShader triVert = XSShader( "Render/Shaders/vert.spv" );
    XSShader triFrag = XSShader( "Render/Shaders/frag.spv" );

    VkShaderModule vertShaderMod;
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = triVert.code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>( triVert.code.data() );
        if ( vkCreateShaderModule( _vkDev, &createInfo, nullptr, &vertShaderMod ) != VK_SUCCESS ) 
        {
            printf( "Failed to create Shader Module 1\n" );
            exit( 1 );
        }
    }

    VkShaderModule fragShaderMod;
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = triFrag.code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>( triFrag.code.data() );
        if ( vkCreateShaderModule( _vkDev, &createInfo, nullptr, &fragShaderMod ) != VK_SUCCESS ) 
        {
            printf( "Failed to create Shader Module 2\n" );
            exit( 1 );
        }
    }

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderMod;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderMod;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    XSArray<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>( dynamicStates.size() );
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    if ( vkCreatePipelineLayout( _vkDev, &pipelineLayoutInfo, nullptr, &_vkPipelineLayout ) != VK_SUCCESS ) 
    {
        printf( "Failed to create pipeline layout\n" );
        exit( 1 );
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = _vkPipelineLayout;
    pipelineInfo.renderPass = _vkRenderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    if ( vkCreateGraphicsPipelines( _vkDev, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_vkGraphicsPipeline ) != VK_SUCCESS ) 
    {
        printf( "Failed to create graphics pipeline\n" );
        exit( 1 );
    }

    vkDestroyShaderModule( _vkDev, fragShaderMod, nullptr );
    vkDestroyShaderModule( _vkDev, vertShaderMod, nullptr );
}

void 
XSDisplay::_createFramebuffers() 
{
    _vkSwapChainFramebuffers.resize( _vkSwapChainImageViews.size() );
    printf( "Framebuffers size: %d\n", _vkSwapChainFramebuffers.size() );
    printf( "Swapchain ImageViews size: %d\n", _vkSwapChainImageViews.size() );

    for( int i=0; i<_vkSwapChainImageViews.size(); i++ ) 
    {
        VkImageView attachments[] = { _vkSwapChainImageViews[i] };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = _vkRenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = _vkSwapChainExtent.width;
        framebufferInfo.height = _vkSwapChainExtent.height;
        framebufferInfo.layers = 1;

        if ( vkCreateFramebuffer( _vkDev, &framebufferInfo, nullptr, &_vkSwapChainFramebuffers[i] ) != VK_SUCCESS ) 
        {
            printf( "Failed to create framebuffer for image view\n" );
            exit( 1 );
        }
    }
}

void 
XSDisplay::_destroyFramebuffers() 
{
    for( auto framebuffer : _vkSwapChainFramebuffers ) 
    {
        vkDestroyFramebuffer( _vkDev, framebuffer, nullptr );
    }
}

void 
XSDisplay::_createCommandPool() 
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = _graphicsFamily;

    if ( vkCreateCommandPool( _vkDev, &poolInfo, nullptr, &_vkCommandPool ) != VK_SUCCESS ) 
    {
        printf( "Failed to create command Pool\n" );
        exit( 1 );
    }
}

void 
XSDisplay::_createCommandBuffer() 
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = _vkCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if ( vkAllocateCommandBuffers( _vkDev, &allocInfo, &_vkCommandBuffer ) != VK_SUCCESS ) 
    {
        printf( "Failed to allocate command buffer\n" );
        exit( 1 );
    }
}

void 
XSDisplay::recordCommandBuffer( VkCommandBuffer commandBuffer, uint32_t imageIndex ) 
{
    VkCommandBufferBeginInfo commandBufferBeginInfo = {};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = 0;
    
    if( vkBeginCommandBuffer( commandBuffer, &commandBufferBeginInfo ) != VK_SUCCESS )
    {
        printf( "Failed to begin command buffer\n" );
        exit( 1 );
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = _vkRenderPass;
    renderPassInfo.framebuffer = _vkSwapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = _vkSwapChainExtent;
    VkClearValue clearColor = { { { 0.0f, 0.0f, 0.0f, 1.0f } } }; //black
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass( commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );
    vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _vkGraphicsPipeline );

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)_vkSwapChainExtent.width;
    viewport.height = (float)_vkSwapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport( commandBuffer, 0, 1, &viewport );

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = _vkSwapChainExtent;
    vkCmdSetScissor( commandBuffer, 0, 1, &scissor );

    vkCmdDraw( commandBuffer, 3, 1, 0, 0 );
    vkCmdEndRenderPass( commandBuffer );

    if( vkEndCommandBuffer( commandBuffer ) != VK_SUCCESS ) 
    {
        printf( "Failed to end command buffer\n" );
        exit( 1 );
    }
}

void 
XSDisplay::_createSyncObjects() 
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if( vkCreateSemaphore( _vkDev, &semaphoreInfo, nullptr, &_imageAvailableSemaphore ) != VK_SUCCESS || vkCreateSemaphore( _vkDev, &semaphoreInfo, nullptr, &_renderFinishedSemaphore) != VK_SUCCESS || vkCreateFence( _vkDev, &fenceInfo, nullptr, &_inFlightFence ) != VK_SUCCESS ) 
    {
        printf( "Failed to create semaphores\n" );
        exit( 1 );
    }
}

void
XSDisplay::drawFrame() 
{
    vkWaitForFences( _vkDev, 1, &_inFlightFence, VK_TRUE, UINT64_MAX );
    vkResetFences( _vkDev, 1, &_inFlightFence );

    uint32_t imageIndex;
    if( vkAcquireNextImageKHR( _vkDev, _vkSwapChain, UINT64_MAX, _imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex ) !=  VK_SUCCESS )
    {
        printf( "Failed to aquire next image\n" );
        exit( 1 );
    }

    vkResetCommandBuffer( _vkCommandBuffer, 0 );
    recordCommandBuffer( _vkCommandBuffer, imageIndex );

    VkSemaphore waitSemaphores[] = { _imageAvailableSemaphore };
    VkSemaphore signalSemaphores[] = { _renderFinishedSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_vkCommandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if( vkQueueSubmit( _graphicsQueue, 1, &submitInfo, _inFlightFence ) != VK_SUCCESS ) 
    {
        printf( "Failed to submit draw queue\n" );
        exit( 1 );
    }

    VkSwapchainKHR swapChains[] = { _vkSwapChain };

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional
    vkQueuePresentKHR( _graphicsQueue, &presentInfo );
}

void 
XSDisplay::_initVulkan() 
{
    _createInstance();
    _pickPhyDevice();
    _createSurface();
    _createXSRDev();
    _createSwapChain();
    _createImageViews();
    _createRenderPass();
    _createGraphicsPipeline();
    _createFramebuffers();
    _createCommandPool();
    _createCommandBuffer();
    _createSyncObjects();
}

void
XSDisplay::_cleanUp() 
{
    vkDestroySemaphore( _vkDev, _imageAvailableSemaphore, nullptr );
    vkDestroySemaphore( _vkDev, _renderFinishedSemaphore, nullptr );
    vkDestroyFence( _vkDev, _inFlightFence, nullptr );
    vkDestroyCommandPool( _vkDev, _vkCommandPool, nullptr );
    _destroyFramebuffers();
    vkDestroyPipeline( _vkDev, _vkGraphicsPipeline, nullptr );
    vkDestroyPipelineLayout( _vkDev, _vkPipelineLayout, nullptr );
    vkDestroyRenderPass( _vkDev, _vkRenderPass, nullptr );
    _destroyImageViews();
    vkDestroySwapchainKHR( _vkDev, _vkSwapChain, nullptr );
    vkDestroySurfaceKHR( _vkInstance, _vkSurface, nullptr );
    vkDestroyDevice( _vkDev, nullptr );
    vkDestroyInstance( _vkInstance, nullptr );
}

XSDisplay::~XSDisplay()
{
    _cleanUp();
}