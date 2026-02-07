#pragma once
#include <vulkan/vulkan_raii.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


class FDisplay
{
public:
	void Init();

private:
	void CreateSurface();
	void CreateSwapChain();
	vk::Extent2D ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
	void CreateSwapChainImageViews();
	vk::raii::ImageView CreateImageView(const vk::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags);
	void CreateSyncObjects();
	static vk::Format ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
	static vk::PresentModeKHR ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);


	class GLFWwindow* Window = nullptr;
	vk::raii::SurfaceKHR Surface;
	vk::Format SwapChainImageFormat;
	vk::Extent2D SwapChainExtent;
	vk::raii::SwapchainKHR SwapChain;
	std::vector<vk::Image> SwapChainImages;
	std::vector<vk::raii::ImageView> SwapChainImageViews;

	std::vector<vk::raii::Semaphore> PresentCompleteSemaphores;
	std::vector<vk::raii::Semaphore> RenderFinishedSemaphores;
	std::vector<vk::raii::Fence> InFlightFences;

	uint32_t CurrentFrame = 0;
	uint32_t SemaphoreIndex = 0;
};