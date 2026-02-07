#include "Display.h"
#include "VulkanContext.h"

void FDisplay::Init()
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	Window = glfwCreateWindow(800, 800, "Vulkan", nullptr, nullptr);
	CreateSurface();
	CreateSwapChain();
	CreateSwapChainImageViews();
	CreateSyncObjects();
}

void FDisplay::CreateSurface()
{
	VkSurfaceKHR _surface;
	if (glfwCreateWindowSurface(*FVulkanStatic::Context->Instance, Window, nullptr, &_surface) != 0)
	{
		throw std::runtime_error("failed to create window surface!");
	}
	Surface = vk::raii::SurfaceKHR(FVulkanStatic::Context->Instance, _surface);
}

void FDisplay::CreateSwapChain() {
	auto surfaceCapabilities = FVulkanStatic::Context->PhysicalDevice.getSurfaceCapabilitiesKHR(Surface);
	SwapChainImageFormat = ChooseSwapSurfaceFormat(FVulkanStatic::Context->PhysicalDevice.getSurfaceFormatsKHR(Surface));
	SwapChainExtent = ChooseSwapExtent(surfaceCapabilities);
	auto minImageCount = std::max(2u, surfaceCapabilities.minImageCount);
	minImageCount = (surfaceCapabilities.maxImageCount > 0 && minImageCount > surfaceCapabilities.maxImageCount)
		? surfaceCapabilities.maxImageCount : minImageCount;
	vk::SwapchainCreateInfoKHR swapChainCreateInfo;
	swapChainCreateInfo.surface = Surface,
		swapChainCreateInfo.minImageCount = 2,
		swapChainCreateInfo.imageFormat = SwapChainImageFormat,
		swapChainCreateInfo.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear,
		swapChainCreateInfo.imageExtent = SwapChainExtent,
		swapChainCreateInfo.imageArrayLayers = 1,
		swapChainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
		swapChainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive,
		swapChainCreateInfo.preTransform = surfaceCapabilities.currentTransform,
		swapChainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
		swapChainCreateInfo.presentMode =
		ChooseSwapPresentMode(FVulkanStatic::Context->PhysicalDevice.getSurfacePresentModesKHR(Surface)),
		swapChainCreateInfo.clipped = true;

	SwapChain = vk::raii::SwapchainKHR(FVulkanStatic::Context->Device, swapChainCreateInfo);
	SwapChainImages = SwapChain.getImages();
}

vk::Extent2D FDisplay::ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	int width, height;
	glfwGetFramebufferSize(Window, &width, &height);

	return {
		std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
		std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
	};
}

void FDisplay::CreateSwapChainImageViews() {
	for (auto Image : SwapChainImages)
	{
		SwapChainImageViews.emplace_back(CreateImageView(Image, SwapChainImageFormat, vk::ImageAspectFlagBits::eColor));
	}
}

vk::raii::ImageView FDisplay::CreateImageView(const vk::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags) {
	vk::ImageViewCreateInfo viewInfo({}, image, vk::ImageViewType::e2D, format, {}, { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	return vk::raii::ImageView(FVulkanStatic::Context->Device, viewInfo);
}

void FDisplay::CreateSyncObjects() {
	PresentCompleteSemaphores.clear();
	RenderFinishedSemaphores.clear();
	InFlightFences.clear();

	for (size_t i = 0; i < FVulkanStatic::Context->MaxFramesInFlight; i++) {
		PresentCompleteSemaphores.emplace_back(FVulkanStatic::Context->Device, vk::SemaphoreCreateInfo());
		RenderFinishedSemaphores.emplace_back(FVulkanStatic::Context->Device, vk::SemaphoreCreateInfo());
		InFlightFences.emplace_back(FVulkanStatic::Context->Device, vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
	}
}

vk::Format FDisplay::ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
	const auto formatIt = std::ranges::find_if(availableFormats,
		[](const auto& format) {
			return format.format == vk::Format::eB8G8R8A8Srgb &&
				format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
		});
	return formatIt != availableFormats.end() ? formatIt->format : availableFormats[0].format;
}

vk::PresentModeKHR FDisplay::ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
	return std::ranges::any_of(availablePresentModes,
		[](const vk::PresentModeKHR value) { return vk::PresentModeKHR::eMailbox == value; }) ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo;
}
