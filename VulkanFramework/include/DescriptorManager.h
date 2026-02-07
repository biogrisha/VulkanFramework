#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <Buffer.h>
#include <LocId.h>
#include <ComponentInterface/ComponentInterface.h>
#include <ImageBuffer.h>
#include <BufferBase.h>

struct FDescriptor
{
	vk::DescriptorType DescriptorType;
	vk::ShaderStageFlags ShaderStages;
};

struct FDescriptorSetLayout
{
	std::vector<FDescriptor> Descriptors;
};

struct FPipelineLayout
{
	std::vector<uint16_t> DescriptorSetLayoutIds;
};

struct FDescriptorBinding
{
	FBufferBase* Buffer;
	FDescriptor Descriptor;
};

struct FDescriptorSet
{
	int16_t LayoutId = -1;
	std::vector<FDescriptorBinding> Bindings;
};


class FDescriptorManager
{
public:
	~FDescriptorManager();
	void Init();
	uint16_t MakePipelineLayout(const std::vector<uint16_t>& DescriptorSetIds);
	uint16_t MakeDescriptorSet(const std::vector<std::pair<FBufferBase*, vk::ShaderStageFlags>>& Bindings);
	vk::PipelineLayout GetPipelineLayout(uint16_t LayoutId);
	std::vector<vk::DescriptorSet> GetDescriptorSets(const std::vector<uint16_t>& SetIds);
private:
	void OnSizeUpdated(FBufferBase* Buffer);
	void RebindBuffer(FBuffer* Buffer);
	void RebindImage(FImageBuffer* Image);
	std::vector<FDescriptorSetLayout> DescriptorSetLayoutDatas;
	std::vector<FPipelineLayout> PipelineLayoutDatas;
	std::vector<FDescriptorSet> DescriptorSetDatas;

	vk::raii::DescriptorPool DescriptorPool = nullptr;
	std::vector<vk::raii::DescriptorSetLayout> DescriptorSetLayouts;
	std::vector<vk::raii::PipelineLayout> PipelineLayouts;
	std::vector<vk::raii::DescriptorSet> DescriptorSets;
	
	std::vector<boost::signals2::connection> OnResizeConnections;
};