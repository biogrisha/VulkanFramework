#include "Rendering.h"
#include <VulkanHelpers.h>

FDescriptorManager& FRendering::GetDescriptorManager()
{
	return DescriptorManager;
}

uint16_t FRendering::AddPipeline(uint16_t PipelineLayout, FVertexInputLayout* VertexInputLayout, const std::string& ShaderPath, vk::Format ColorAttachmentFormat)
{
	auto& Pipeline = Pipelines.emplace_back();
	Pipeline.SetVertexInputLayout(VertexInputLayout);
	Pipeline.SetColorAttachmentFormat(ColorAttachmentFormat);
	Pipeline.SetPipelineLayout(DescriptorManager.GetPipelineLayout(PipelineLayout));
	Pipeline.SetShaderPath(ShaderPath);
	Pipeline.Init();
	return Pipelines.size() - 1;
}

void FRendering::RunPipeline(const FRunPipelineInfo& RunPipelineInfo)
{
	//Set clear color
	vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f);
	//Set draw region
	auto OutputExtent = RunPipelineInfo.OutputExtent;

	//Set attachment info
	vk::RenderingAttachmentInfo ResultAttachmentInfo;
	ResultAttachmentInfo.imageView = RunPipelineInfo.ColorAttachment->GetImageView();
	ResultAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
	ResultAttachmentInfo.loadOp = RunPipelineInfo.bClearAttachment ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
	ResultAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
	ResultAttachmentInfo.clearValue = clearColor;

	//Set info about rendering and what color attachment to use
	vk::RenderingInfo ResultRenderingInfo;
	ResultRenderingInfo.renderArea = { .offset = { 0, 0 }, .extent = OutputExtent };
	ResultRenderingInfo.layerCount = 1;
	ResultRenderingInfo.colorAttachmentCount = 1;
	ResultRenderingInfo.pColorAttachments = &ResultAttachmentInfo;

	//Transit color attachment layout to be color attachment optimal
	VkImageMemoryBarrier PreResultBarrier{};
	PreResultBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	PreResultBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	PreResultBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	PreResultBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	PreResultBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	PreResultBarrier.image = RunPipelineInfo.ColorAttachment->GetImage();
	PreResultBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	PreResultBarrier.subresourceRange.baseMipLevel = 0;
	PreResultBarrier.subresourceRange.levelCount = 1;
	PreResultBarrier.subresourceRange.baseArrayLayer = 0;
	PreResultBarrier.subresourceRange.layerCount = 1;

	// Transit attachment access from shader read to attachment write
	PreResultBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	PreResultBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	//Begin command buffer
	auto commandBuffer = VkHelpers::BeginSingleTimeCommands();
	vkCmdPipelineBarrier(
		*commandBuffer,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,          // srcStageMask (or TOP_OF_PIPE if UNDEFINED)
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // dstStageMask
		0,
		0, nullptr,
		0, nullptr,
		1, &PreResultBarrier
	);
	//set rendering info
	commandBuffer.beginRendering(ResultRenderingInfo);
	//bind pipeline
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, Pipelines[RunPipelineInfo.PipelineId].GetPipeline());
	//set the region to render into
	commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(OutputExtent.width), static_cast<float>(OutputExtent.height), 0.0f, 1.0f));
	commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), OutputExtent));

	VkDeviceSize offset = 0;
	//set vertex buffers
	for(int i = 0; i < RunPipelineInfo.VertexBuffers.size(); i++)
	{
		vkCmdBindVertexBuffers(*commandBuffer, i, 1, RunPipelineInfo.VertexBuffers[i]->GetBuffer(), &offset);
	}
	//set index buffer
	vkCmdBindIndexBuffer(*commandBuffer, *RunPipelineInfo.IndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT16);

	//bind descriptor sets
	commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, Pipelines[RunPipelineInfo.PipelineId].GetPipelineLayout(), 0
		, DescriptorManager.GetDescriptorSets(RunPipelineInfo.DescriptorSets), nullptr);
	//draw command
	commandBuffer.drawIndexed(RunPipelineInfo.IndicesCount, RunPipelineInfo.InstancesCount, 0, 0, 0);
	commandBuffer.endRendering();

	//convert color attachment back into shader read
	VkImageMemoryBarrier PostBarrier{};
	PostBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	PostBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	PostBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	PostBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	PostBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	PostBarrier.image = RunPipelineInfo.ColorAttachment->GetImage();
	PostBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	PostBarrier.subresourceRange.baseMipLevel = 0;
	PostBarrier.subresourceRange.levelCount = 1;
	PostBarrier.subresourceRange.baseArrayLayer = 0;
	PostBarrier.subresourceRange.layerCount = 1;

	PostBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	PostBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(
		*commandBuffer,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // srcStageMask
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,         // dstStageMask
		0,
		0, nullptr,
		0, nullptr,
		1, &PostBarrier
	);
	//submit command buffer on the queue
	VkHelpers::EndSingleTimeCommands(commandBuffer);
}

void FRendering::AddRunPipelineInfo(const FRunPipelineInfo& RunPipelineInfo)
{
	RunPipelineInfoArray.push_back(std::make_unique<FRunPipelineInfo>(RunPipelineInfo));
}

void FRendering::Render()
{
	for (auto& RunPipelineInfo : RunPipelineInfoArray)
	{
		RunPipeline(*RunPipelineInfo.get());
	}
	RunPipelineInfoArray.clear();
}
