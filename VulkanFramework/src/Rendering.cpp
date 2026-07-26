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
	ResultRenderingInfo.renderArea = { .offset = { 0, 0 }, .extent = vk::Extent2D(OutputExtent.width, OutputExtent.height) };
	ResultRenderingInfo.layerCount = 1;
	ResultRenderingInfo.colorAttachmentCount = 1;
	ResultRenderingInfo.pColorAttachments = &ResultAttachmentInfo;


	//Begin command buffer
	auto commandBuffer = VkHelpers::BeginSingleTimeCommands();
	//Transit color attachment layout to be color attachment optimal
	VkHelpers::ImageTransition_ToCollorAttachment(RunPipelineInfo.ColorAttachment, commandBuffer);


	auto dSets = DescriptorManager.descriptorSetDatas();
	for (uint16_t dSetId : RunPipelineInfo.DescriptorSets)
	{
		for (auto& binding : dSets[dSetId].Bindings)
		{
			if (binding.Descriptor.DescriptorType == vk::DescriptorType::eCombinedImageSampler)
			{
				if (auto img = MyRTTI::Cast<FImageBuffer>(binding.Buffer))
				{
					if (img->GetLayout() != vk::ImageLayout::eShaderReadOnlyOptimal)
					{
						VkHelpers::ImageTransition_ToShaderRead(img, commandBuffer);
					}
				}
			}
		}
	}
	//set rendering info
	commandBuffer.beginRendering(ResultRenderingInfo);
	//bind pipeline
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, Pipelines[RunPipelineInfo.PipelineId].GetPipeline());
	//set the region to render into
	commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(OutputExtent.width), static_cast<float>(OutputExtent.height), 0.0f, 1.0f));
	commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(OutputExtent.width, OutputExtent.height)));

	VkDeviceSize offset = 0;
	//set vertex buffers
	for (int i = 0; i < RunPipelineInfo.VertexBuffers.size(); i++)
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
