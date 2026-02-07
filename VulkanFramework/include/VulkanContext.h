#pragma once
#include <vulkan/vulkan_raii.hpp>

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <vulkan/vk_platform.h>

#define GLFW_INCLUDE_VULKAN
#include "VmaUsage.h"
#include <algorithm>
#include <vector>
#include <iostream>

class FVulkanContext
{
public:
	~FVulkanContext();
	void Init(VkInstance InInstance, VkPhysicalDevice InPhysicalDevice);
	void CreateLogicalDevice();

private:
	void CreateVmaAllocator();
	void CreateCommandPool();

	//Device
	std::vector<const char*> RequiredDeviceExtension = {
		vk::KHRSwapchainExtensionName,
		vk::KHRSpirv14ExtensionName,
		vk::KHRSynchronization2ExtensionName,
		vk::KHRCreateRenderpass2ExtensionName,
		vk::KHRDynamicRenderingExtensionName,
		vk::KHRShaderFloatControlsExtensionName,
		vk::KHRDepthStencilResolveExtensionName
	};
public:

	uint32_t GraphicsQueueInd = 0;
	VmaAllocator VmaAllocator;

	vk::raii::Context Context;
	vk::raii::Instance Instance = nullptr;
	vk::raii::PhysicalDevice PhysicalDevice = nullptr;
	vk::raii::Queue GraphicsQueue = nullptr;
	vk::raii::CommandPool CommandPool = nullptr;
	vk::raii::Device Device = nullptr;
};

class FVulkanStatic
{
public:
	static void SubscribeToContext(VkInstance InInstance, VkPhysicalDevice InPhysicalDevice);
	static void UnsubscribeFromContext();
	static inline std::shared_ptr<FVulkanContext> Context = nullptr;
private:
	static inline uint32_t UserCount = 0;
};