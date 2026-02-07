#pragma once
#include <BufferBase.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_raii.hpp>
#include <VmaUsage.h>
#include <boost/signals2.hpp>

class FImageBuffer : public FBufferBase
{
	TYPED_CLASS1(FBufferBase)
public:
	~FImageBuffer();
	void SetExtent(const VkExtent2D& InExtent);
	void SetFormat(VkFormat InFormat);
	void AddUsageFlags(VkImageUsageFlags Flags);
	void Init();
	void UpdateImageFromData(void* InDataPointer);

	VkImage GetImage();
	vk::ImageView GetImageView();
	vk::Sampler GetSampler();
	VkExtent2D GetExtent();
	vk::DescriptorImageInfo* GetDescriptorImageInfo();
private:
	void DestroyImage();
	VkImage Image = nullptr;
	VmaAllocation Allocation = nullptr;
	vk::raii::ImageView ImageView = nullptr;
	vk::raii::Sampler Sampler = nullptr;
	VkExtent2D Extent = {0, 0};
	VkFormat Format = VkFormat::VK_FORMAT_B8G8R8A8_UNORM;
	vk::DescriptorImageInfo DescriptorImageInfo;
	VkImageUsageFlags ImageUsageFlags = 0;
};