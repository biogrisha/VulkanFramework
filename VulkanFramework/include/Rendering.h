#pragma once
#include "Pipeline.h"
#include "DescriptorManager.h"
#include "VertexInputLayout.h"

struct FRunPipelineInfo
{
	uint16_t				PipelineId = UINT16_MAX;
	vk::Extent2D			OutputExtent = {0,0};
	std::vector<FBuffer*>	VertexBuffers;
	FBuffer*				IndexBuffer = nullptr;
	std::vector<uint16_t>	DescriptorSets;
	FImageBuffer*			ColorAttachment = nullptr;
	uint16_t				IndicesCount = 0;
	uint16_t				InstancesCount = 0;
	bool					bClearAttachment = true;
};

class FRendering
{
public:
	FDescriptorManager& GetDescriptorManager();
	uint16_t AddPipeline(uint16_t PipelineLayout, FVertexInputLayout* VertexInputLayout, const std::string& ShaderPath, vk::Format ColorAttachmentFormat = vk::Format::eB8G8R8A8Unorm);
	void AddRunPipelineInfo(const FRunPipelineInfo& RunPipelineInfo);
	void Render();
private:
	void RunPipeline(const FRunPipelineInfo& RunPipelineInfo);
	FDescriptorManager DescriptorManager;
	std::vector<FPipeline> Pipelines;
	std::vector <std::unique_ptr<FRunPipelineInfo>> RunPipelineInfoArray;
};