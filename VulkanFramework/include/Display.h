#pragma once
#include <vulkan/vulkan_raii.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

struct GLFWwindow;
class FImageBuffer;

class FDisplayRender
{
public:
	virtual FImageBuffer* Render() = 0;
	virtual void SetResolution(int InWidth, int InHeight) = 0;
};


class FDisplay
{
public:
	~FDisplay();
	void Init(FDisplayRender* InDisplayRender);
	GLFWwindow* GetWindow();
	void DrawFrame();
private:
	void CreateSurface();
	void CreateSwapChain();
	vk::Extent2D ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
	void CreateSwapChainImageViews();
	vk::raii::ImageView CreateImageView(const vk::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags);
	void CreateSyncObjects();
	void RecreateSwapChain();
	void ClearSwapChain();
	void CreateCommandBuffers();
	void FindPresentQueue();
	static vk::Format ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);

	GLFWwindow* Window = nullptr;
	vk::raii::SurfaceKHR Surface = nullptr;
	vk::Format SwapChainImageFormat;
	vk::Extent2D SwapChainExtent;
	vk::raii::SwapchainKHR SwapChain = nullptr;
	vk::raii::Queue PresentQueue = nullptr;

	std::vector<vk::Image> SwapChainImages;
	std::vector<vk::raii::ImageView> SwapChainImageViews;

	std::vector<vk::raii::Semaphore> PresentCompleteSemaphores;
	std::vector<vk::raii::Semaphore> RenderFinishedSemaphores;
	std::vector<vk::raii::Fence> InFlightFences;
	std::vector<vk::raii::CommandBuffer> CommandBuffers;

	uint32_t CurrentFrame = 0;
	uint32_t SemaphoreIndex = 0;

	uint32_t Width = 800;
	uint32_t Height = 800;

	FDisplayRender* DisplayRender = nullptr;
public:
	bool bFrameBufferResized = false;
};