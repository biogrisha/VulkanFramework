#include "VulkanHelpers.h"
#include "VulkanContext.h"
#include <fstream>
vk::raii::ShaderModule VkHelpers::createShaderModule(const std::vector<char>& code, const vk::raii::Device& device)
{
	vk::ShaderModuleCreateInfo createInfo;
	createInfo.codeSize = code.size() * sizeof(char), createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	vk::raii::ShaderModule shaderModule{ device, createInfo };

	return shaderModule;
}

std::vector<char> VkHelpers::readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}
	std::vector<char> buffer(file.tellg());
	file.seekg(0, std::ios::beg);
	file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
	file.close();
	return buffer;
}

vk::raii::CommandBuffer VkHelpers::BeginSingleTimeCommands()
{
	vk::CommandBufferAllocateInfo allocInfo(FVulkanStatic::Context->CommandPool, vk::CommandBufferLevel::ePrimary, 1);
	vk::raii::CommandBuffer commandBuffer = std::move(FVulkanStatic::Context->Device.allocateCommandBuffers(allocInfo).front());

	vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	commandBuffer.begin(beginInfo);

	return commandBuffer;
}

void VkHelpers::EndSingleTimeCommands(vk::CommandBuffer commandBuffer)
{
	commandBuffer.end();

	vk::SubmitInfo submitInfo({}, {}, { commandBuffer });
	FVulkanStatic::Context->GraphicsQueue.submit(submitInfo, nullptr);
	FVulkanStatic::Context->GraphicsQueue.waitIdle();
}

void VkHelpers::CopyBufferRaw(VkBuffer* srcBuffer, VkBuffer* dstBuffer, vk::DeviceSize size)
{
	vk::raii::CommandBuffer commandCopyBuffer = BeginSingleTimeCommands();
	commandCopyBuffer.copyBuffer(*srcBuffer, *dstBuffer, vk::BufferCopy(0, 0, size));
	EndSingleTimeCommands(commandCopyBuffer);
}

void VkHelpers::TransitionImageLayout(const vk::raii::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
	auto commandBuffer = BeginSingleTimeCommands();
	vk::ImageMemoryBarrier barrier({}, {}, oldLayout, newLayout, {}, {}, image, { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

	vk::PipelineStageFlags sourceStage;
	vk::PipelineStageFlags destinationStage;

	if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
		barrier.srcAccessMask = {};
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		sourceStage = vk::PipelineStageFlagBits::eTransfer;
		destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}
	commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, {}, barrier);
	EndSingleTimeCommands(commandBuffer);
}

void VkHelpers::CopyBufferToImage(FBuffer* Buffer, FImageBuffer* ImageBuffer, const vk::raii::CommandBuffer& CommandBuffer)
{
	auto ImageExtent = ImageBuffer->GetExtent();
	vk::BufferImageCopy region;
	region.bufferImageHeight = 0;
	region.bufferRowLength = 0;
	region.bufferOffset = 0;
	region.imageExtent = vk::Extent3D{ ImageExtent.width, ImageExtent.height, 1 };
	region.imageOffset = vk::Offset3D{ 0, 0, 0 };
	region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	CommandBuffer.copyBufferToImage(*Buffer->GetBuffer(), ImageBuffer->GetImage(), vk::ImageLayout::eTransferDstOptimal, region);
}

void VkHelpers::CopyImageToImage(FImageBuffer* Src, FImageBuffer* Dst, const vk::raii::CommandBuffer& CommandBuffer)
{
	vk::ImageCopy Copy;
	Copy.srcOffset = vk::Offset3D{ 0,0,0 };
	Copy.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	Copy.srcSubresource.mipLevel = 0;
	Copy.srcSubresource.baseArrayLayer = 0;
	Copy.srcSubresource.layerCount = 1;
	auto Extent = Src->GetExtent();
	Copy.extent = vk::Extent3D{ Extent.width,Extent.height, 1 };
	Copy.dstOffset = vk::Offset3D{ 0,0,0 };
	Copy.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	Copy.dstSubresource.mipLevel = 0;
	Copy.dstSubresource.baseArrayLayer = 0;
	Copy.dstSubresource.layerCount = 1;
	CommandBuffer.copyImage(Src->GetImage(), vk::ImageLayout::eTransferSrcOptimal, Dst->GetImage(), vk::ImageLayout::eTransferDstOptimal, Copy);
}

void VkHelpers::ClearImage(FImageBuffer* Image, const vk::raii::CommandBuffer& CommandBuffer)
{
	vk::ClearColorValue Color;
	Color.setFloat32({ 0,0,0,0 });
	vk::ImageSubresourceRange Range;
	Range.aspectMask = vk::ImageAspectFlagBits::eColor;
	Range.baseMipLevel = 0;
	Range.levelCount = 1;
	Range.baseArrayLayer = 0;
	Range.layerCount = 1;
	CommandBuffer.clearColorImage(Image->GetImage(), vk::ImageLayout::eTransferDstOptimal, Color, Range);
}

vk::DescriptorType VkHelpers::ConvertBufferToDescriptor(VkBufferUsageFlagBits BufferUsage)
{
	if (BufferUsage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
		return vk::DescriptorType::eUniformBuffer;

	if (BufferUsage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
		return vk::DescriptorType::eStorageBuffer;

	// Optional: handle unsupported usage
	throw std::runtime_error("Unsupported buffer usage for descriptor type.");
}

std::unique_ptr<FBuffer> VkHelpers::ConvertImageToBuffer(FImageBuffer* ImageBuffer, const vk::raii::CommandBuffer& CommandBuffer)
{
	std::unique_ptr<FBuffer> Buffer = MyRTTI::MakeTypedUnique<FBuffer>();
	FBufferInfo BufferInfo;
	BufferInfo.bDeviceLocal = false;
	BufferInfo.Usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	Buffer->SetProperties(BufferInfo);
	auto ImageExtent = ImageBuffer->GetExtent();
	Buffer->InitBuffer(ImageExtent.height * ImageExtent.width * 4);

	vk::BufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = vk::Offset3D{ 0, 0, 0 };
	region.imageExtent = vk::Extent3D{ ImageExtent.width, ImageExtent.height, 1 };

	CommandBuffer.copyImageToBuffer(ImageBuffer->GetImage(), vk::ImageLayout::eTransferSrcOptimal, *Buffer->GetBuffer(), region);

	return Buffer;
}

void VkHelpers::ImageTransitionGeneral(FImageBuffer* Image, const vk::raii::CommandBuffer& CommandBuffer,
	vk::ImageLayout NewLayout,
	vk::AccessFlags DstAccess,
	vk::PipelineStageFlags DstStage)
{
	//Fill memory barrier info
	vk::ImageMemoryBarrier barrier{};
	barrier.image = Image->GetImage();

	barrier.oldLayout = Image->GetLayout();
	barrier.newLayout = NewLayout;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	barrier.srcAccessMask = Image->GetAccess();
	barrier.dstAccessMask = DstAccess;

	CommandBuffer.pipelineBarrier(
		Image->stage(),
		DstStage,
		{},
		nullptr,
		nullptr,
		barrier
	);
	Image->SetLayout(NewLayout);
	Image->SetAccess(DstAccess);
	Image->setStage(DstStage);
}

void VkHelpers::ImageTransition_ToTransferSrc(FImageBuffer* Image, const vk::raii::CommandBuffer& CommandBuffer)
{
	ImageTransitionGeneral(
		Image,
		CommandBuffer,
		vk::ImageLayout::eTransferSrcOptimal,
		vk::AccessFlagBits::eTransferRead,
		vk::PipelineStageFlagBits::eTransfer
	);
}

void VkHelpers::ImageTransition_ToTransferDst(FImageBuffer* Image, const vk::raii::CommandBuffer& CommandBuffer)
{
	ImageTransitionGeneral(
		Image,
		CommandBuffer,
		vk::ImageLayout::eTransferDstOptimal,
		vk::AccessFlagBits::eTransferWrite,
		vk::PipelineStageFlagBits::eTransfer
	);
}

void VkHelpers::ImageTransition_ToShaderRead(FImageBuffer* Image, const vk::raii::CommandBuffer& CommandBuffer)
{
	ImageTransitionGeneral(
		Image,
		CommandBuffer,
		vk::ImageLayout::eShaderReadOnlyOptimal,
		vk::AccessFlagBits::eShaderRead,
		vk::PipelineStageFlagBits::eFragmentShader
	);
}

void VkHelpers::ImageTransition_ToCollorAttachment(FImageBuffer* Image, const vk::raii::CommandBuffer& CommandBuffer)
{
	ImageTransitionGeneral(
		Image,
		CommandBuffer,
		vk::ImageLayout::eColorAttachmentOptimal,
		vk::AccessFlagBits::eColorAttachmentWrite,
		vk::PipelineStageFlagBits::eColorAttachmentOutput
	);
}

vk::ImageViewType VkHelpers::ToImageViewType(vk::ImageType type, bool isArray)
{
	switch (type)
	{
	case vk::ImageType::e1D:
		return isArray ? vk::ImageViewType::e1DArray : vk::ImageViewType::e1D;

	case vk::ImageType::e2D:
		return isArray ? vk::ImageViewType::e2DArray : vk::ImageViewType::e2D;

	case vk::ImageType::e3D:
		return vk::ImageViewType::e3D;

	default:
		throw std::runtime_error("Unsupported image type");
	}
}

VkShaderModule NVkHelpers::createShaderModule(const uint32_t* code, uint32_t size, VkDevice device)
{
	VkShaderModuleCreateInfo createInfo;
	memset(&createInfo, 0, sizeof(createInfo));
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.flags = 0;
	createInfo.codeSize = size;
	createInfo.pCode = code;
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}
	return shaderModule;
}