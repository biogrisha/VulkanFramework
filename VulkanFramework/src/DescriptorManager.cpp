#include "DescriptorManager.h"
#include <array>
#include <unordered_map>
#include <boost/signals2.hpp>
#include <boost/bind.hpp>
#include <boost/bind/placeholders.hpp>
#include <VulkanContext.h>
#include <algorithm>
#include <VulkanHelpers.h>

FDescriptorManager::~FDescriptorManager()
{
	for (auto& Connection : OnResizeConnections)
	{
		Connection.disconnect();
	}
}
void FDescriptorManager::Init()
{
	for (auto& SetLayoutData : DescriptorSetLayoutDatas)
	{
		std::vector<vk::DescriptorSetLayoutBinding> Descriptors;
		for (size_t DescriptorId = 0; DescriptorId < SetLayoutData.Descriptors.size(); DescriptorId++)
		{
			auto& DescriptorData = SetLayoutData.Descriptors[DescriptorId];
			auto& Descriptor = Descriptors.emplace_back();
			Descriptor.binding = DescriptorId;
			Descriptor.descriptorCount = 1;
			Descriptor.stageFlags = DescriptorData.ShaderStages;
			Descriptor.descriptorType = DescriptorData.DescriptorType;
		}
		vk::DescriptorSetLayoutCreateInfo layoutInfo({}, Descriptors.size(), Descriptors.data());
		DescriptorSetLayouts.emplace_back(FVulkanStatic::Context->Device, layoutInfo);
	}

	for (auto& PipelineLayoutData : PipelineLayoutDatas)
	{
		std::vector<vk::DescriptorSetLayout> PLineDescriptorSetLayouts;
		for (auto& LayoutId : PipelineLayoutData.DescriptorSetLayoutIds)
		{
			PLineDescriptorSetLayouts.push_back(DescriptorSetLayouts[LayoutId]);
		}
		vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.setLayoutCount = PLineDescriptorSetLayouts.size();
		pipelineLayoutInfo.pSetLayouts = PLineDescriptorSetLayouts.data();
		PipelineLayouts.emplace_back(FVulkanStatic::Context->Device, pipelineLayoutInfo);
	}
	
	// Count descriptors by type
	std::unordered_map<vk::DescriptorType, uint32_t> descriptorCounts;
	for (auto& DescriptorSetData : DescriptorSetDatas)
	{
		auto DescriptorSetLayout = DescriptorSetLayoutDatas[DescriptorSetData.LayoutId];
		for (auto& DescriptorData : DescriptorSetLayout.Descriptors)
		{
			descriptorCounts[DescriptorData.DescriptorType] += 1;
		}
	}

	// Convert counts into pool sizes
	std::vector<vk::DescriptorPoolSize> PoolSizes;
	PoolSizes.reserve(descriptorCounts.size());

	for (auto& [type, count] : descriptorCounts) {
		PoolSizes.emplace_back(type, count);
	}

	vk::DescriptorPoolCreateInfo PoolInfo(
		vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
		DescriptorSetDatas.size(), // maxSets
		static_cast<uint32_t>(PoolSizes.size()),
		PoolSizes.data()
	);

	DescriptorPool = vk::raii::DescriptorPool(FVulkanStatic::Context->Device, PoolInfo);

	for (auto& DescriptorSetData : DescriptorSetDatas)
	{
		vk::DescriptorSetAllocateInfo AllocInfo;
		AllocInfo.descriptorPool = DescriptorPool;
		AllocInfo.descriptorSetCount = 1;
		AllocInfo.pSetLayouts = &*(DescriptorSetLayouts[DescriptorSetData.LayoutId]);
		DescriptorSets.emplace_back(std::move(FVulkanStatic::Context->Device.allocateDescriptorSets(AllocInfo).back()));
	}

	std::vector<vk::WriteDescriptorSet> Writes;
	std::set<FBufferBase*> DistinctBuffers;
	int DescriptorSetId = 0;
	for (auto& DescriptorSetData : DescriptorSetDatas)
	{
		auto& Bindings = DescriptorSetData.Bindings;
		for (uint32_t BindingId = 0; BindingId < Bindings.size(); BindingId++)
		{
			DistinctBuffers.insert(Bindings[BindingId].Buffer);
			if (!Bindings[BindingId].Buffer->IsInitialized())
			{
				continue;
			}

			vk::WriteDescriptorSet Write{ DescriptorSets[DescriptorSetId], BindingId, 0, 1
					, Bindings[BindingId].Descriptor.DescriptorType };

			if (auto Buffer = MyRTTI::Cast<FBuffer>(Bindings[BindingId].Buffer))
			{
				Write.setPBufferInfo(Buffer->GetDescriptorBufferInfo());
			}
			else if (auto Image = MyRTTI::Cast<FImageBuffer>(Bindings[BindingId].Buffer))
			{
				Write.setPImageInfo(Image->GetDescriptorImageInfo());
			}
			Writes.push_back(Write);
		}
		DescriptorSetId++;
	}

	FVulkanStatic::Context->Device.updateDescriptorSets(Writes, {});

	for (FBufferBase* Buffer : DistinctBuffers)
	{
		auto Connection = Buffer->OnSizeUpdated.connect(boost::bind(&FDescriptorManager::OnSizeUpdated, this, boost::placeholders::_1));
		OnResizeConnections.push_back(Connection);
	}
}

uint16_t FDescriptorManager::MakePipelineLayout(const std::vector<uint16_t>& DescriptorSetIds)
{
	auto& NewPLineLayout = PipelineLayoutDatas.emplace_back();
	for (auto& DescriptorSetId : DescriptorSetIds)
	{
		NewPLineLayout.DescriptorSetLayoutIds.push_back(DescriptorSetDatas[DescriptorSetId].LayoutId);
	}
	return PipelineLayoutDatas.size() - 1;
}

uint16_t FDescriptorManager::MakeDescriptorSet(const std::vector<std::pair<FBufferBase*, vk::ShaderStageFlags>>& Bindings)
{
	FDescriptorSet Set;
	//converting input pairs into descriptor bindings
	for (auto& Pair : Bindings)
	{
		//creating new binding and set bound buffer
		auto& NewBinding = Set.Bindings.emplace_back();
		NewBinding.Buffer = Pair.first;
		NewBinding.Descriptor.ShaderStages = Pair.second;

		if (FBuffer* Buffer = MyRTTI::Cast<FBuffer>(Pair.first))
		{
			//binding buffer
			//set descriptor type from buffer usage
			NewBinding.Descriptor.DescriptorType = VkHelpers::ConvertBufferToDescriptor(Buffer->GetBufferInfo().Usage);
		}
		else
		{
			//binding image
			//use combined sampler for descriptor type
			NewBinding.Descriptor.DescriptorType = vk::DescriptorType::eCombinedImageSampler;
		}
	}

	//searching for existing layout for descriptor set
	for (size_t LayoutId = 0; LayoutId < DescriptorSetLayoutDatas.size(); LayoutId++)
	{
		auto& Descriptors = DescriptorSetLayoutDatas[LayoutId].Descriptors;
		if(Descriptors.size() == Bindings.size())
		{
			//found layout of same size
			bool bFoundLayout = true;
			//comparing descriptors
			for (size_t DescrId = 0; DescrId < Descriptors.size(); DescrId++)
			{
				if (Descriptors[DescrId].DescriptorType != Set.Bindings[DescrId].Descriptor.DescriptorType
					|| Descriptors[DescrId].ShaderStages != Set.Bindings[DescrId].Descriptor.ShaderStages)
				{
					//descriptors are different
					//try next layout
					bFoundLayout = false;
					break;
				}
			}
			if (bFoundLayout)
			{
				//descriptors are same
				//use this layout id
				Set.LayoutId = LayoutId;
				break;
			}
		}
	}

	if (Set.LayoutId == -1)
	{
		//no layout was found
		//add new one
		auto& NewLayout = DescriptorSetLayoutDatas.emplace_back();
		for (auto& Binding : Set.Bindings)
		{
			NewLayout.Descriptors.push_back(Binding.Descriptor);
		}
		Set.LayoutId = DescriptorSetLayoutDatas.size() - 1;
	}
	DescriptorSetDatas.push_back(Set);
	return DescriptorSetDatas.size() - 1;
}

vk::PipelineLayout FDescriptorManager::GetPipelineLayout(uint16_t LayoutId)
{
	return PipelineLayouts[LayoutId];
}

std::vector<vk::DescriptorSet> FDescriptorManager::GetDescriptorSets(const std::vector<uint16_t>& SetIds)
{
	std::vector<vk::DescriptorSet> Result;
	for (auto Id : SetIds)
	{
		Result.push_back(DescriptorSets[Id]);
	}
	return Result;
}

void FDescriptorManager::OnSizeUpdated(FBufferBase* Buffer)
{
	if (auto AsBuffer = MyRTTI::Cast<FBuffer>(Buffer))
	{
		RebindBuffer(AsBuffer);
	}
	else if (auto AsImage = MyRTTI::Cast<FImageBuffer>(Buffer))
	{
		RebindImage(AsImage);
	}
}

void FDescriptorManager::RebindBuffer(FBuffer* Buffer)
{
	std::vector<vk::WriteDescriptorSet> Writes;
	int DescriptorSetId = 0;

	for (auto& DescriptorSetData : DescriptorSetDatas)
	{
		//iterate Descriptor sets data
		auto& Bindings = DescriptorSetData.Bindings;
		for (uint32_t BindingId = 0; BindingId < Bindings.size(); BindingId++)
		{
			//Iterate bindings
			if(Bindings[BindingId].Buffer == Buffer)
			{
				//found buffer in bound buffers
				//create descriptor set write for it
				vk::WriteDescriptorSet Write{ DescriptorSets[DescriptorSetId], BindingId, 0, 1
						, Bindings[BindingId].Descriptor.DescriptorType };

				Write.setPBufferInfo(Buffer->GetDescriptorBufferInfo());
				Writes.push_back(Write);
			}
		}
	}

	FVulkanStatic::Context->Device.updateDescriptorSets(Writes, {});
}

void FDescriptorManager::RebindImage(FImageBuffer* Image)
{
	std::vector<vk::WriteDescriptorSet> Writes;
	int DescriptorSetId = 0;

	for (auto& DescriptorSetData : DescriptorSetDatas)
	{
		//iterate Descriptor sets data
		auto& Bindings = DescriptorSetData.Bindings;
		for (uint32_t BindingId = 0; BindingId < Bindings.size(); BindingId++)
		{
			//Iterate bindings
			if (Bindings[BindingId].Buffer == Image)
			{
				//found buffer in bound buffers
				//create descriptor set write for it
				vk::WriteDescriptorSet Write{ DescriptorSets[DescriptorSetId], BindingId, 0, 1
						, Bindings[BindingId].Descriptor.DescriptorType };

				Write.setPImageInfo(Image->GetDescriptorImageInfo());
				Writes.push_back(Write);
			}
		}
		DescriptorSetId++;
	}

	FVulkanStatic::Context->Device.updateDescriptorSets(Writes, {});
}
