#include "Display.h"
#include "VulkanContext.h"
#include "ImageBuffer.h"
#include "VulkanHelpers.h"

FDisplay::~FDisplay()
{
	glfwDestroyWindow(Window);
}

static void FramebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	auto App = reinterpret_cast<FDisplay*>(glfwGetWindowUserPointer(window));
	App->bFrameBufferResized = true;
}
void FDisplay::Init(FDisplayRender* InDisplayRender)
{
	DisplayRender = InDisplayRender;
	DisplayRender->SetResolution(800, 800);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	Window = glfwCreateWindow(800, 800, "Vulkan", nullptr, nullptr);
	glfwSetWindowUserPointer(Window, this);
	glfwSetFramebufferSizeCallback(Window, FramebufferResizeCallback);
	CreateSurface();
	CreateSwapChain();
	CreateSwapChainImageViews();
	CreateSyncObjects();
	CreateCommandBuffers();
	FindPresentQueue();
}

GLFWwindow* FDisplay::GetWindow()
{
	return Window;
}

void FDisplay::DrawFrame()
{
	//wait until graphics queue finished for this frame
	while (vk::Result::eTimeout == FVulkanStatic::Context->Device.waitForFences(*InFlightFences[CurrentFrame], vk::True, UINT64_MAX))
		;
	if (bFrameBufferResized) {
		RecreateSwapChain();
		bFrameBufferResized = false;
		return;
	}
	//acquire new free frame
	auto [result, imageIndex] = SwapChain.acquireNextImage(UINT64_MAX, *PresentCompleteSemaphores[SemaphoreIndex], nullptr);

	if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	//reset graphics fence
	FVulkanStatic::Context->Device.resetFences(*InFlightFences[CurrentFrame]);

	//write command buffer
	if(bFrameBufferResized)
	{
		DisplayRender->SetResolution(Width, Height);
	}

	FImageBuffer* SrcImage = DisplayRender->Render();
	auto img = SwapChainImages[imageIndex].operator VkImage();

	FImageBufferInfo Info;
	Info.Extent = vk::Extent3D{ Width, Height, 1 };
	Info.UsageFlags = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;

	FImageBuffer DstImage(img, vk::ImageLayout::eUndefined, vk::AccessFlagBits::eNone, Info);
	CommandBuffers[CurrentFrame].reset();
	vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	CommandBuffers[CurrentFrame].begin(beginInfo);
	VkHelpers::ImageTransitionGeneral(&DstImage, CommandBuffers[CurrentFrame],
		vk::ImageLayout::eTransferDstOptimal,
		vk::AccessFlagBits::eTransferWrite,
		vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eTransfer);

	VkHelpers::ImageTransitionGeneral(SrcImage, CommandBuffers[CurrentFrame],
		vk::ImageLayout::eTransferSrcOptimal,
		vk::AccessFlagBits::eTransferRead,
		vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eTransfer);

	VkHelpers::CopyImageToImage(SrcImage, &DstImage, CommandBuffers[CurrentFrame]);

	VkHelpers::ImageTransitionGeneral(&DstImage, CommandBuffers[CurrentFrame],
		vk::ImageLayout::ePresentSrcKHR,
		{},
		vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eBottomOfPipe);

	VkHelpers::ImageTransitionGeneral(SrcImage, CommandBuffers[CurrentFrame],
		vk::ImageLayout::eShaderReadOnlyOptimal,
		vk::AccessFlagBits::eShaderRead,
		vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader);

	CommandBuffers[CurrentFrame].end();
	//submit commands info to run after present semaphore and signal rendering semaphore
	vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
	vk::SubmitInfo submitInfo;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &*PresentCompleteSemaphores[SemaphoreIndex];
	submitInfo.pWaitDstStageMask = &waitDestinationStageMask;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &*CommandBuffers[CurrentFrame];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &*RenderFinishedSemaphores[imageIndex];

	//submit graphics
	FVulkanStatic::Context->GraphicsQueue.submit(submitInfo, *InFlightFences[CurrentFrame]);
	FVulkanStatic::Context->GraphicsQueue.waitIdle();
	//present info to run after rendering semaphore
	vk::PresentInfoKHR presentInfoKHR;
	presentInfoKHR.waitSemaphoreCount = 1;
	presentInfoKHR.pWaitSemaphores = &*RenderFinishedSemaphores[imageIndex];
	presentInfoKHR.swapchainCount = 1;
	presentInfoKHR.pSwapchains = &*SwapChain;
	presentInfoKHR.pImageIndices = &imageIndex;

	//present command
	result = PresentQueue.presentKHR(presentInfoKHR);


	if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || bFrameBufferResized) {
		//surface incompatible with swap chain
		//recreate swap chain
		bFrameBufferResized = false;
		RecreateSwapChain();
	}
	else if (result != vk::Result::eSuccess) {
		throw std::runtime_error("failed to present swap chain image!");
	}
	SemaphoreIndex = (SemaphoreIndex + 1) % PresentCompleteSemaphores.size();
	CurrentFrame = (CurrentFrame + 1) % FVulkanStatic::Context->MaxFramesInFlight;
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
	//cache surface properties
	auto SurfaceCapabilities = FVulkanStatic::Context->PhysicalDevice.getSurfaceCapabilitiesKHR(Surface);

	SwapChainImageFormat = ChooseSwapSurfaceFormat(FVulkanStatic::Context->PhysicalDevice.getSurfaceFormatsKHR(Surface));
	SwapChainExtent = ChooseSwapExtent(SurfaceCapabilities);

	vk::SwapchainCreateInfoKHR swapChainCreateInfo;
	swapChainCreateInfo.surface = Surface;
	swapChainCreateInfo.minImageCount = 2;
	swapChainCreateInfo.imageFormat = SwapChainImageFormat;
	swapChainCreateInfo.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
	swapChainCreateInfo.imageExtent = SwapChainExtent;
	swapChainCreateInfo.imageArrayLayers = 1;
	swapChainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
	swapChainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
	swapChainCreateInfo.preTransform = SurfaceCapabilities.currentTransform;
	swapChainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	swapChainCreateInfo.presentMode = vk::PresentModeKHR::eFifo;
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

void FDisplay::RecreateSwapChain() {

	int WidthTmp, HeightTmp = 0;
	glfwGetFramebufferSize(Window, &WidthTmp, &HeightTmp);
	while (Width == 0 || Height == 0) {
		glfwGetFramebufferSize(Window, &WidthTmp, &HeightTmp);
		glfwWaitEvents();
	}
	Width = WidthTmp;
	Height = HeightTmp;
	FVulkanStatic::Context->Device.waitIdle();

	ClearSwapChain();
	CreateSwapChain();
	CreateSwapChainImageViews();
}

void FDisplay::ClearSwapChain()
{
	SwapChainImageViews.clear();
	SwapChain = nullptr;
}

void FDisplay::CreateCommandBuffers() {
	vk::CommandBufferAllocateInfo AllocInfo;
	AllocInfo.commandPool = FVulkanStatic::Context->CommandPool;
	AllocInfo.level = vk::CommandBufferLevel::ePrimary;
	AllocInfo.commandBufferCount = FVulkanStatic::Context->MaxFramesInFlight;
	CommandBuffers = vk::raii::CommandBuffers(FVulkanStatic::Context->Device, AllocInfo);
}

void FDisplay::FindPresentQueue()
{
	// find the index of the first queue family that supports graphics
	std::vector<vk::QueueFamilyProperties> QueueFamilyProperties = FVulkanStatic::Context->PhysicalDevice.getQueueFamilyProperties();

	uint32_t PresentIndex = 0;
	for (size_t i = 0; i < QueueFamilyProperties.size(); i++)
	{
		if (FVulkanStatic::Context->PhysicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), *Surface))
		{
			PresentIndex = static_cast<uint32_t>(i);;
			break;
		}
	}

	PresentQueue = vk::raii::Queue(FVulkanStatic::Context->Device, PresentIndex, 0);
}

vk::Format FDisplay::ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
	const auto formatIt = std::ranges::find_if(availableFormats,
		[](const auto& format) {
			return format.format == vk::Format::eB8G8R8A8Srgb &&
				format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
		});
	return formatIt != availableFormats.end() ? formatIt->format : availableFormats[0].format;
}
