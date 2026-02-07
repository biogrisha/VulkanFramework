#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <vector>
#include <string>
#include <DescriptorManager.h>

#include <iostream>

#define LOG_TEST() \
    std::cout << "TEST " << __FILE__ << " " << __LINE__ << std::endl;

namespace VkHelpers
{ 
	[[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code, const vk::raii::Device& device);

	std::vector<char> readFile(const std::string& filename);

	vk::raii::CommandBuffer BeginSingleTimeCommands();

	void EndSingleTimeCommands(vk::CommandBuffer commandBuffer);

	void CopyBufferRaw(VkBuffer* srcBuffer, VkBuffer* dstBuffer, vk::DeviceSize size);

	void TransitionImageLayout(const vk::raii::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);

	void CopyBufferToImage(FBuffer* Buffer, FImageBuffer* ImageBuffer, const vk::raii::CommandBuffer& CommandBuffer);

	void CopyImageToImage(FImageBuffer* Src, FImageBuffer* Dst, const vk::raii::CommandBuffer& CommandBuffer);

	void ClearImage(FImageBuffer* Image, const vk::raii::CommandBuffer& CommandBuffer);

	vk::DescriptorType ConvertBufferToDescriptor(VkBufferUsageFlagBits BufferUsage);

	std::unique_ptr<FBuffer> ConvertImageToBuffer(FImageBuffer* ImageBuffer);

	void ImageTransition_ShaderReadToTransferSrc(FImageBuffer* Image, const vk::raii::CommandBuffer& CommandBuffer);

	void ImageTransition_UnknownToTransferDst(FImageBuffer* Image, const vk::raii::CommandBuffer& CommandBuffer);

	void ImageTransition_TransferSrcToShaderRead(FImageBuffer* Image, const vk::raii::CommandBuffer& CommandBuffer);

	void ImageTransition_TransferDstToShaderRead(FImageBuffer* Image, const vk::raii::CommandBuffer& CommandBuffer);
}

namespace NVkHelpers
{
	[[nodiscard]] VkShaderModule createShaderModule(const uint32_t* code,uint32_t size, VkDevice device);
}

