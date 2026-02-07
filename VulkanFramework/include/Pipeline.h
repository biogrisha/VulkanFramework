#pragma once
#include <iostream>
#include <vulkan/vulkan_raii.hpp>
#include <VertexInputLayout.h>

class FPipeline {
public:
	void SetShaderPath(const std::string& path);
	void SetVertexInputLayout(FVertexInputLayout* layout);
	void SetColorAttachmentFormat(vk::Format format);
	void SetPipelineLayout(const vk::PipelineLayout& InPipelineLayout);
	void Init();

	vk::Pipeline GetPipeline();
	vk::PipelineLayout GetPipelineLayout();
private:
	std::string shaderPath;
	FVertexInputLayout* vertexInputLayout = nullptr;
	vk::Format colorAttachmentFormat = vk::Format::eB8G8R8A8Srgb;
	vk::PipelineLayout PipelineLayout;
	vk::raii::Pipeline pipeline = nullptr;
};