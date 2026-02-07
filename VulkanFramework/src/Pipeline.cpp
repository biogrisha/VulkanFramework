#include "Pipeline.h"
#include "VulkanHelpers.h"
#include "VulkanContext.h"

void FPipeline::Init() {

	//create shaders for pipeline
	vk::raii::ShaderModule Module = VkHelpers::createShaderModule(VkHelpers::readFile(shaderPath), FVulkanStatic::Context->Device);
	vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
	vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex, vertShaderStageInfo.module = Module, vertShaderStageInfo.pName = "vertMain";
	vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
	fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment, fragShaderStageInfo.module = Module, fragShaderStageInfo.pName = "fragMain";
	vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	//vertex binding and attribute descriptions

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
	auto bindingDescription = vertexInputLayout->getBindingDescription();
	auto attributeDescriptions = vertexInputLayout->getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = bindingDescription.size();
	vertexInputInfo.pVertexBindingDescriptions = bindingDescription.data();
	vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	// Primitive topology type
	vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
	inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;

	//viewport and scissor rectangles
	vk::PipelineViewportStateCreateInfo viewportState;
	viewportState.viewportCount = 1, viewportState.scissorCount = 1;

	// rasterizer configuration
	vk::PipelineRasterizationStateCreateInfo rasterizer({}, vk::False, vk::False, vk::PolygonMode::eFill,
		vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise, vk::False, 0.0f, 0.0f, 1.0f, 1.0f);

	//multisampling configuration
	vk::PipelineMultisampleStateCreateInfo multisampling;
	multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1, multisampling.sampleShadingEnable = vk::False;

	//color output and blending configuration
	vk::PipelineColorBlendAttachmentState colorBlendAttachment;
	colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR
		| vk::ColorComponentFlagBits::eG
		| vk::ColorComponentFlagBits::eB
		| vk::ColorComponentFlagBits::eA;
	colorBlendAttachment.blendEnable = vk::True;
	colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
	colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
	colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
	colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
	colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;

	vk::PipelineColorBlendStateCreateInfo colorBlending;
	colorBlending.logicOpEnable = vk::False;
	colorBlending.logicOp = vk::LogicOp::eCopy;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	//dynamic states configuration
	std::vector dynamicStates = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor
	};
	vk::PipelineDynamicStateCreateInfo dynamicState;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	//configure attachments format
	vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo;
	pipelineRenderingCreateInfo.colorAttachmentCount = 1;
	pipelineRenderingCreateInfo.pColorAttachmentFormats = &colorAttachmentFormat;

	vk::raii::PipelineLayout Layout = vk::raii::PipelineLayout(FVulkanStatic::Context->Device, PipelineLayout);
	if (!*Layout)
	{
		vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		Layout = vk::raii::PipelineLayout(FVulkanStatic::Context->Device, pipelineLayoutInfo);
	}
	vk::GraphicsPipelineCreateInfo pipelineInfo;
	pipelineInfo.pNext = &pipelineRenderingCreateInfo;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = PipelineLayout;
	pipelineInfo.renderPass = nullptr;

	pipeline = vk::raii::Pipeline(FVulkanStatic::Context->Device, nullptr, pipelineInfo);
	Layout.release();
}

void FPipeline::SetShaderPath(const std::string& path)
{
	shaderPath = path;
}

void FPipeline::SetVertexInputLayout(FVertexInputLayout* layout)
{
	vertexInputLayout = layout;
}

void FPipeline::SetColorAttachmentFormat(vk::Format format)
{
	colorAttachmentFormat = format;
}

vk::Pipeline FPipeline::GetPipeline()
{
	return *pipeline;
}

void FPipeline::SetPipelineLayout(const vk::PipelineLayout& InPipelineLayout)
{
	PipelineLayout = InPipelineLayout;
}

vk::PipelineLayout FPipeline::GetPipelineLayout()
{
	return PipelineLayout;
}

