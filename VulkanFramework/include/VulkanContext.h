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
	void Init(bool bEnableValidationLayer);
	void Init(VkInstance InInstance, VkPhysicalDevice InPhysicalDevice, VkDevice device);
	void CreateLogicalDevice();

private:
	void CreateInstance(bool bEnableValidationLayers);
	void CreateVmaAllocator();
	void CreateCommandPool();
	void SetupDebugMessenger(bool bEnableValidationLayer);
	void PickPhysicalDevice();
	void findGraphicsQueueInd();
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
	bool m_foreign = false;
	uint32_t GraphicsQueueInd = 0;
	VmaAllocator VmaAllocator;
	const int MaxFramesInFlight = 2;

	vk::raii::Context Context;
	vk::raii::Instance Instance = nullptr;
	vk::raii::PhysicalDevice PhysicalDevice = nullptr;
	vk::raii::Device Device = nullptr;
	vk::raii::Queue GraphicsQueue = nullptr;
	vk::raii::CommandPool CommandPool = nullptr;
	vk::raii::DebugUtilsMessengerEXT DebugMessenger = nullptr;
};

class FVulkanStatic
{
public:
	static void InitContext();
	static void InitContext(VkInstance InInstance, VkPhysicalDevice InPhysicalDevice, VkDevice device);
	static void ClearContext();
	static inline std::unique_ptr<FVulkanContext> Context = nullptr;
};