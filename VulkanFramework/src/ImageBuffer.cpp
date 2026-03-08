#include "ImageBuffer.h"
#include "ImageBuffer.h"
#include "ImageBuffer.h"
#include "ImageBuffer.h"
#include "ImageBuffer.h"
#include <VulkanContext.h>
#include <VulkanHelpers.h>

FImageBuffer::~FImageBuffer()
{
	DestroyImage();
}

FImageBuffer::FImageBuffer(const FImageBufferInfo& ImageBufferInfo)
{
	Info = ImageBufferInfo;
	Info.UsageFlags |= vk::ImageUsageFlagBits::eSampled;
}

FImageBuffer::FImageBuffer(VkImage ExternalImage, vk::ImageLayout ImageLayout, vk::AccessFlags ImageAccess, const FImageBufferInfo& ImageBufferInfo)
{
	bExternal = true;
	Image = ExternalImage;
	Info = ImageBufferInfo;
	Layout = ImageLayout;
	Access = ImageAccess;
}

void FImageBuffer::SetExtent(const vk::Extent3D& InExtent)
{
	if (InExtent != Info.Extent)
	{
		Info.Extent = InExtent;
		if(bInitialized)
		{
			auto OldAllocation = Allocation;
			auto OldImage = Image;
			Init();
			OnSizeUpdated(this);
			vmaDestroyImage(FVulkanStatic::Context->VmaAllocator, OldImage, OldAllocation);
		}
	}
}

void FImageBuffer::Init()
{
	//Fill create info
	VkImageCreateInfo ImageCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	ImageCreateInfo.format = static_cast<VkFormat>(Info.Format);
	ImageCreateInfo.imageType = static_cast<VkImageType>(Info.Type);
	ImageCreateInfo.extent = static_cast<VkExtent3D>(Info.Extent);
	ImageCreateInfo.mipLevels = 1;
	ImageCreateInfo.arrayLayers = 1;
	ImageCreateInfo.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
	ImageCreateInfo.tiling = VkImageTiling::VK_IMAGE_TILING_OPTIMAL;
	ImageCreateInfo.usage = static_cast<VkImageUsageFlags>(Info.UsageFlags);
	ImageCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
	ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	//Fill allocation info
	//Allocate on device
	VmaAllocationCreateInfo AllocInfo = {};
	AllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	AllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	//Create image
	vmaCreateImage(FVulkanStatic::Context->VmaAllocator, &ImageCreateInfo, &AllocInfo, &Image, &Allocation, nullptr);

	//Fill image view info
	vk::ImageViewCreateInfo ViewInfo({}, Image, VkHelpers::ToImageViewType(Info.Type), Info.Format, {}, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
	ImageView = vk::raii::ImageView(FVulkanStatic::Context->Device, ViewInfo);

	//Fill sampler info
	vk::PhysicalDeviceProperties PhysicalDeviceProperties = FVulkanStatic::Context->PhysicalDevice.getProperties();
	vk::SamplerCreateInfo SamplerInfo({}, vk::Filter::eLinear, vk::Filter::eLinear,
		vk::SamplerMipmapMode::eLinear, vk::SamplerAddressMode::eRepeat,
		vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, 0, 1,
		PhysicalDeviceProperties.limits.maxSamplerAnisotropy, vk::False, vk::CompareOp::eAlways);

	SamplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
	SamplerInfo.unnormalizedCoordinates = vk::False;
	SamplerInfo.compareEnable = vk::False;
	SamplerInfo.compareOp = vk::CompareOp::eAlways;
	SamplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
	SamplerInfo.mipLodBias = 0.0f;
	SamplerInfo.minLod = 0.0f;
	SamplerInfo.maxLod = 0.0f;

	//Create sampler
	Sampler = vk::raii::Sampler(FVulkanStatic::Context->Device, SamplerInfo);

	//Start command (transfer undefined->shader read only)
	auto CommandBuffer = VkHelpers::BeginSingleTimeCommands();

	//Fill memory barrier info
	vk::ImageMemoryBarrier barrier{};
	barrier.image = Image;

	barrier.oldLayout = vk::ImageLayout::eUndefined;
	barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	barrier.srcAccessMask = vk::AccessFlags{};
	barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

	CommandBuffer.pipelineBarrier(
		vk::PipelineStageFlagBits::eTopOfPipe,
		vk::PipelineStageFlagBits::eFragmentShader,
		{},
		nullptr,
		nullptr,
		barrier
	);

	VkHelpers::EndSingleTimeCommands(CommandBuffer);
	Layout = vk::ImageLayout::eShaderReadOnlyOptimal;
	Access = vk::AccessFlagBits::eShaderRead;
	//Cache descriptor info
	DescriptorImageInfo = vk::DescriptorImageInfo(Sampler, ImageView, vk::ImageLayout::eShaderReadOnlyOptimal);

	bInitialized = true;
}

void FImageBuffer::UpdateImageFromData(void* InDataPointer)
{
	assert(Info.Type == vk::ImageType::e2D);

	//Fill buffer info
	FBufferInfo BufferInfo;
	BufferInfo.bDeviceLocal = false;
	BufferInfo.Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	//Calculate buffer size
	uint32_t BufferSize = Info.Extent.height * Info.Extent.width * 4;

	//Create staging buffer for image data
	auto StagingBuffer = MyRTTI::MakeTypedUnique<FBuffer>();
	StagingBuffer->SetProperties(BufferInfo);
	StagingBuffer->SetData(BufferSize, InDataPointer);

	//Copy buffer into image
	auto CommandBuffer = VkHelpers::BeginSingleTimeCommands();
	VkHelpers::ImageTransition_ToTransferDst(this, CommandBuffer,{});
	VkHelpers::CopyBufferToImage(StagingBuffer.get(), this, CommandBuffer);
	VkHelpers::ImageTransition_ToShaderRead(this, CommandBuffer, vk::PipelineStageFlagBits::eTransfer);
	VkHelpers::EndSingleTimeCommands(CommandBuffer);
}

void FImageBuffer::DestroyImage()
{
	if (bExternal)
	{
		return;
	}
	vmaDestroyImage(FVulkanStatic::Context->VmaAllocator, Image, Allocation);
}

VkImage FImageBuffer::GetImage()
{
	return Image;
}

vk::ImageView FImageBuffer::GetImageView()
{
	return ImageView;
}

vk::Sampler FImageBuffer::GetSampler()
{
	return Sampler;
}

VkExtent3D FImageBuffer::GetExtent()
{
	return static_cast<VkExtent3D>(Info.Extent);
}

vk::DescriptorImageInfo* FImageBuffer::GetDescriptorImageInfo()
{
	return &DescriptorImageInfo;
}

vk::ImageLayout FImageBuffer::GetLayout()
{
	return Layout;
}

vk::AccessFlags FImageBuffer::GetAccess()
{
	return Access;
}

void FImageBuffer::SetLayout(vk::ImageLayout InLayout)
{
	Layout = InLayout;
}

void FImageBuffer::SetAccess(vk::AccessFlags InAccess)
{
	Access = InAccess;
}
