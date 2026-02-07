#include <VulkanContext.h>


FVulkanContext::~FVulkanContext()
{
	vmaDestroyAllocator(VmaAllocator);
	CommandPool.clear();
	GraphicsQueue.clear();
	Device.clear();
	PhysicalDevice.release();
	Instance.release();
}

void FVulkanContext::Init(VkInstance InInstance, VkPhysicalDevice InPhysicalDevice)
{
	Instance = vk::raii::Instance(Context, InInstance);
	PhysicalDevice = vk::raii::PhysicalDevice(Instance, InPhysicalDevice);
	CreateLogicalDevice();
	GraphicsQueue = vk::raii::Queue(Device, GraphicsQueueInd, 0);
	CreateVmaAllocator();
	CreateCommandPool();
}

void FVulkanContext::CreateLogicalDevice()
{
	// find the index of the first queue family that supports graphics
	std::vector<vk::QueueFamilyProperties> queueFamilyProperties = PhysicalDevice.getQueueFamilyProperties();

	// get the first index into queueFamilyProperties which supports graphics
	auto graphicsQueueFamilyProperty = std::ranges::find_if(queueFamilyProperties, [](auto const& qfp)
		{ return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0); });

	auto GraphicsQueueIndex = static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(), graphicsQueueFamilyProperty));

	// query for Vulkan 1.3 features
	vk::PhysicalDeviceVulkan13Features PhysicalDeviceVulkan13Features;
	PhysicalDeviceVulkan13Features.synchronization2 = true;
	PhysicalDeviceVulkan13Features.dynamicRendering = true;

	vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT PhysicalDeviceExtendedDynamicStateFeaturesEXT;
	PhysicalDeviceExtendedDynamicStateFeaturesEXT.extendedDynamicState = true;

	vk::PhysicalDeviceFeatures2 physicalDeviceFeatures2;
	physicalDeviceFeatures2.features.samplerAnisotropy = true;
	physicalDeviceFeatures2.features.logicOp = true;

	vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
		physicalDeviceFeatures2,          
		PhysicalDeviceVulkan13Features,  
		PhysicalDeviceExtendedDynamicStateFeaturesEXT
	};

	// create a Device
	float queuePriority = 0.0f;
	vk::DeviceQueueCreateInfo deviceQueueCreateInfo;
	deviceQueueCreateInfo.queueFamilyIndex = GraphicsQueueIndex;
	deviceQueueCreateInfo.queueCount = 1;
	deviceQueueCreateInfo.pQueuePriorities = &queuePriority;

	vk::DeviceCreateInfo deviceCreateInfo;
	deviceCreateInfo.pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>();
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(RequiredDeviceExtension.size());
	deviceCreateInfo.ppEnabledExtensionNames = RequiredDeviceExtension.data();

	Device = vk::raii::Device(PhysicalDevice, deviceCreateInfo);
}

void FVulkanContext::CreateVmaAllocator()
{
	VmaVulkanFunctions vulkanFunctions = {};
	vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
	vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = *PhysicalDevice;
	allocatorInfo.device = *Device;
	allocatorInfo.instance = *Instance;
	allocatorInfo.vulkanApiVersion = vk::ApiVersion14;
	auto Result = vmaCreateAllocator(&allocatorInfo, &VmaAllocator);
	if (Result != VK_SUCCESS) {
		throw std::runtime_error("failed to create VMA allocator!");
	}
}

void FVulkanContext::CreateCommandPool()
{
	vk::CommandPoolCreateInfo poolInfo;
	poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	poolInfo.queueFamilyIndex = GraphicsQueueInd;
	CommandPool = vk::raii::CommandPool(Device, poolInfo);
}

void FVulkanStatic::SubscribeToContext(VkInstance InInstance, VkPhysicalDevice InPhysicalDevice)
{
	if (!Context.get())
	{
		Context = std::make_unique<FVulkanContext>();
		Context->Init(InInstance, InPhysicalDevice);
		UserCount++;
	}
}

void FVulkanStatic::UnsubscribeFromContext()
{
	UserCount--;
	if (UserCount <= 0)
	{
		Context.reset();
	}
}


