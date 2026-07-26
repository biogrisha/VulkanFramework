#pragma once
#include <BufferBase.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_raii.hpp>
#include <VmaUsage.h>
#include <boost/signals2.hpp>

struct FImageBufferInfo
{
	vk::ImageUsageFlags UsageFlags;
	vk::Extent3D Extent = { 800,800,1 };
	vk::Format Format = vk::Format::eB8G8R8A8Unorm;
	vk::ImageType Type = vk::ImageType::e2D;
};

class FImageBuffer : public FBufferBase
{
	TYPED_CLASS1(FBufferBase)
public:
	~FImageBuffer();
	FImageBuffer(const FImageBufferInfo& ImageBufferInfo);
	FImageBuffer(VkImage ExternalImage, vk::ImageLayout ImageLayout, vk::AccessFlags ImageAccess, const FImageBufferInfo& ImageBufferInfo);
	void SetExtent(const vk::Extent3D& InExtent);
	void UpdateImageFromData(void* InDataPointer);

	VkImage GetImage();
	vk::ImageView GetImageView();
	vk::Sampler GetSampler();
	VkExtent3D GetExtent();
	vk::DescriptorImageInfo* GetDescriptorImageInfo();
	vk::ImageLayout GetLayout() const;
	vk::AccessFlags GetAccess() const;
	vk::PipelineStageFlags stage() const;
	void SetLayout(vk::ImageLayout InLayout);
	void SetAccess(vk::AccessFlags InAccess);
	void setStage(vk::PipelineStageFlags stage);
private:
	void Init();
	void DestroyImage();
	VkImage Image = nullptr;
	VmaAllocation Allocation = nullptr;

	vk::raii::ImageView ImageView = nullptr;
	vk::raii::Sampler Sampler = nullptr;
	vk::DescriptorImageInfo DescriptorImageInfo;

	bool bExternal = false;
	FImageBufferInfo Info;

	vk::ImageLayout Layout = vk::ImageLayout::eUndefined;
	vk::AccessFlags Access = vk::AccessFlagBits::eNone;;
	vk::PipelineStageFlags m_stage = vk::PipelineStageFlagBits::eFragmentShader;
};