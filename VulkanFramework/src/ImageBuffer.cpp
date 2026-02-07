#include "ImageBuffer.h"
#include <VulkanContext.h>
#include <VulkanHelpers.h>
FImageBuffer::~FImageBuffer()
{
	DestroyImage();
}

void FImageBuffer::SetExtent(const VkExtent2D& InExtent)
{
	if (Extent.height != InExtent.height || Extent.width != InExtent.width)
	{
		Extent = InExtent;
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

void FImageBuffer::SetFormat(VkFormat InFormat)
{
	Format = InFormat;
}

void FImageBuffer::AddUsageFlags(VkImageUsageFlags Flags)
{
	ImageUsageFlags = Flags;
}

void FImageBuffer::Init()
{
	VkImageCreateInfo ImageCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	ImageCreateInfo.format = Format;
	ImageCreateInfo.imageType = VkImageType::VK_IMAGE_TYPE_2D;
	ImageCreateInfo.extent = {Extent.width,Extent.height, 1};
	ImageCreateInfo.mipLevels = 1;
	ImageCreateInfo.arrayLayers = 1;
	ImageCreateInfo.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
	ImageCreateInfo.tiling = VkImageTiling::VK_IMAGE_TILING_OPTIMAL;
	ImageCreateInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT | ImageUsageFlags;
	ImageCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
	ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;


	VmaAllocationCreateInfo AllocInfo = {};
	AllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	AllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	vmaCreateImage(FVulkanStatic::Context->VmaAllocator, &ImageCreateInfo, &AllocInfo, &Image, &Allocation, nullptr);

	vk::ImageViewCreateInfo ViewInfo({}, Image, vk::ImageViewType::e2D, vk::Format(Format), {}, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
	ImageView = vk::raii::ImageView(FVulkanStatic::Context->Device, ViewInfo);

	//Creating Sampler
	vk::PhysicalDeviceProperties properties = FVulkanStatic::Context->PhysicalDevice.getProperties();
	vk::SamplerCreateInfo SamplerInfo({}, vk::Filter::eLinear, vk::Filter::eLinear,
		vk::SamplerMipmapMode::eLinear, vk::SamplerAddressMode::eRepeat,
		vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, 0, 1,
		properties.limits.maxSamplerAnisotropy, vk::False, vk::CompareOp::eAlways);

	SamplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
	SamplerInfo.unnormalizedCoordinates = vk::False;
	SamplerInfo.compareEnable = vk::False;
	SamplerInfo.compareOp = vk::CompareOp::eAlways;
	SamplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
	SamplerInfo.mipLodBias = 0.0f;
	SamplerInfo.minLod = 0.0f;
	SamplerInfo.maxLod = 0.0f;

	Sampler = vk::raii::Sampler(FVulkanStatic::Context->Device, SamplerInfo);

	auto CommandBuffer = VkHelpers::BeginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; // or UNDEFINED
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = Image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	// Access masks depend on old usage:
	barrier.srcAccessMask = VK_ACCESS_NONE;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(
		*CommandBuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,          // srcStageMask (or TOP_OF_PIPE if UNDEFINED)
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,			// dstStageMask
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);
	VkHelpers::EndSingleTimeCommands(CommandBuffer);
	DescriptorImageInfo = vk::DescriptorImageInfo(Sampler, ImageView, vk::ImageLayout::eShaderReadOnlyOptimal);
	bInitialized = true;
}

void FImageBuffer::UpdateImageFromData(void* InDataPointer)
{
	uint32_t BufferSize = Extent.height * Extent.width * 4;
	auto StagingBuffer = MyRTTI::MakeTypedUnique<FBuffer>();
	FBufferInfo Info;
	Info.bDeviceLocal = false;
	Info.Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	StagingBuffer->SetProperties(Info);
	StagingBuffer->SetData(BufferSize, InDataPointer);
	auto CommandBuffer = VkHelpers::BeginSingleTimeCommands();
	VkHelpers::ImageTransition_UnknownToTransferDst(this, CommandBuffer);
	VkHelpers::CopyBufferToImage(StagingBuffer.get(), this, CommandBuffer);
	VkHelpers::ImageTransition_TransferDstToShaderRead(this, CommandBuffer);
	VkHelpers::EndSingleTimeCommands(CommandBuffer);
}

void FImageBuffer::DestroyImage()
{
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

VkExtent2D FImageBuffer::GetExtent()
{
	return Extent;
}

vk::DescriptorImageInfo* FImageBuffer::GetDescriptorImageInfo()
{
	return &DescriptorImageInfo;
}
