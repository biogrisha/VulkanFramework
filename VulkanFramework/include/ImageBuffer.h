#pragma once
#include <BufferBase.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_raii.hpp>
#include <VmaUsage.h>
#include <boost/signals2.hpp>

struct FImageBufferInfo
{
	vk::ImageUsageFlags UsageFlags = vk::ImageUsageFlagBits::eColorAttachment;
	vk::Extent3D Extent = {800,800,1};
	vk::Format Format = vk::Format::eB8G8R8A8Unorm;
	vk::ImageType Type = vk::ImageType::e2D;
};

class FImageBuffer : public FBufferBase
{
	TYPED_CLASS1(FBufferBase)
public:
	~FImageBuffer();
	FImageBuffer() = default;
	FImageBuffer(const FImageBufferInfo& ImageBufferInfo);
	FImageBuffer(VkImage ExternalImage, vk::ImageLayout ImageLayout, vk::AccessFlags ImageAccess, const FImageBufferInfo& ImageBufferInfo);
	void SetExtent(const vk::Extent3D& InExtent);
	void Init();
	void UpdateImageFromData(void* InDataPointer);

	VkImage GetImage();
	vk::ImageView GetImageView();
	vk::Sampler GetSampler();
	VkExtent3D GetExtent();
	vk::DescriptorImageInfo* GetDescriptorImageInfo();
	vk::ImageLayout GetLayout();
	vk::AccessFlags GetAccess();
	void SetLayout(vk::ImageLayout InLayout);
	void SetAccess(vk::AccessFlags InAccess);
private:
	void DestroyImage();
	VkImage Image = nullptr;
	VmaAllocation Allocation = nullptr;

	vk::raii::ImageView ImageView = nullptr;
	vk::raii::Sampler Sampler = nullptr;
	vk::DescriptorImageInfo DescriptorImageInfo;

	bool bExternal = false;
	FImageBufferInfo Info;

	vk::ImageLayout Layout = vk::ImageLayout::eUndefined;
	vk::AccessFlags Access = vk::AccessFlagBits::eColorAttachmentRead;
};