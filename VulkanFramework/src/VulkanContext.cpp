#include <VulkanContext.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

FVulkanContext::~FVulkanContext()
{
	vmaDestroyAllocator(VmaAllocator);
	glfwTerminate();
}

std::vector<const char*> getRequiredExtensions(bool bEnableValidationLayers)
{
	uint32_t glfwExtensionCount = 0;
	auto     glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	if (bEnableValidationLayers)
	{
		extensions.push_back(vk::EXTDebugUtilsExtensionName);
	}

	return extensions;
}

void FVulkanContext::CreateInstance(bool bEnableValidationLayers)
{
	constexpr vk::ApplicationInfo appInfo{ "Application",
										  VK_MAKE_VERSION(1, 0, 0),
										  "No Engine",
										  VK_MAKE_VERSION(1, 0, 0),
										  vk::ApiVersion14 };

	// Get the required layers
	std::vector<char const*> requiredLayers;
	if (bEnableValidationLayers)
	{
		const std::vector<char const*> validationLayers = {	"VK_LAYER_KHRONOS_validation" };
		requiredLayers.assign(validationLayers.begin(), validationLayers.end());
	}

	// Check if the required layers are supported by the Vulkan implementation.
	auto layerProperties = Context.enumerateInstanceLayerProperties();
	for (auto const& requiredLayer : requiredLayers)
	{
		if (std::ranges::none_of(layerProperties,
			[requiredLayer](auto const& layerProperty) { return strcmp(layerProperty.layerName, requiredLayer) == 0; }))
		{
			throw std::runtime_error("Required layer not supported: " + std::string(requiredLayer));
		}
	}

	// Get the required extensions.
	auto requiredExtensions = getRequiredExtensions(bEnableValidationLayers);

	// Check if the required extensions are supported by the Vulkan implementation.
	auto extensionProperties = Context.enumerateInstanceExtensionProperties();
	for (auto const& requiredExtension : requiredExtensions)
	{
		if (std::ranges::none_of(extensionProperties,
			[requiredExtension](auto const& extensionProperty) { return strcmp(extensionProperty.extensionName, requiredExtension) == 0; }))
		{
			throw std::runtime_error("Required extension not supported: " + std::string(requiredExtension));
		}
	}

	vk::InstanceCreateInfo createInfo;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size());
	createInfo.ppEnabledLayerNames = requiredLayers.data();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
	createInfo.ppEnabledExtensionNames = requiredExtensions.data();
	Instance = vk::raii::Instance(Context, createInfo);
}

void FVulkanContext::Init(bool bEnableValidationLayer)
{
	glfwInit();
	CreateInstance(bEnableValidationLayer);
	SetupDebugMessenger(bEnableValidationLayer);
	PickPhysicalDevice();
	CreateLogicalDevice();
	GraphicsQueue = vk::raii::Queue(Device, GraphicsQueueInd, 0);
	CreateVmaAllocator();
	CreateCommandPool();
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

static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void*)
{
	if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError || severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
	{
		std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;
	}

	return vk::False;
}

void FVulkanContext::SetupDebugMessenger(bool bEnableValidationLayer)
{
	if (!bEnableValidationLayer)
		return;

	vk::DebugUtilsMessageSeverityFlagsEXT SeverityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
	vk::DebugUtilsMessageTypeFlagsEXT     MessageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
	vk::DebugUtilsMessengerCreateInfoEXT  DebugUtilsMessengerCreateInfoEXT;
	DebugUtilsMessengerCreateInfoEXT.messageSeverity = SeverityFlags;
	DebugUtilsMessengerCreateInfoEXT.messageType = MessageTypeFlags;
	DebugUtilsMessengerCreateInfoEXT.pfnUserCallback = &debugCallback;
	DebugMessenger = Instance.createDebugUtilsMessengerEXT(DebugUtilsMessengerCreateInfoEXT);
}

void FVulkanContext::PickPhysicalDevice()
{
	std::vector<vk::raii::PhysicalDevice> devices = Instance.enumeratePhysicalDevices();
	const auto                            devIter = std::ranges::find_if(
		devices,
		[&](auto const& device) {
			// Check if the device supports the Vulkan 1.3 API version
			bool supportsVulkan1_3 = device.getProperties().apiVersion >= VK_API_VERSION_1_3;

			// Check if any of the queue families support graphics operations
			auto queueFamilies = device.getQueueFamilyProperties();
			bool supportsGraphics =
				std::ranges::any_of(queueFamilies, [](auto const& qfp) { return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics); });

			// Check if all required device extensions are available
			auto availableDeviceExtensions = device.enumerateDeviceExtensionProperties();
			bool supportsAllRequiredExtensions =
				std::ranges::all_of(RequiredDeviceExtension,
					[&availableDeviceExtensions](auto const& RequiredDeviceExtension) {
						return std::ranges::any_of(availableDeviceExtensions,
							[RequiredDeviceExtension](auto const& availableDeviceExtension) { return strcmp(availableDeviceExtension.extensionName, RequiredDeviceExtension) == 0; });
					});

			auto features = device.template getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
			bool supportsRequiredFeatures = features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
				features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

			return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
		});
	if (devIter != devices.end())
	{
		PhysicalDevice = *devIter;
	}
	else
	{
		throw std::runtime_error("failed to find a suitable GPU!");
	}
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

void FVulkanStatic::InitContext()
{
	Context = std::make_unique<FVulkanContext>();
	Context->Init(true);
}

void FVulkanStatic::ClearContext()
{
	Context.reset();
}
