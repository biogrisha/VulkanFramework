#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <boost/signals2.hpp>
#include <VmaUsage.h>
#include <iostream>
#include <BufferBase.h>

struct FBufferInfo
{
	VkBufferUsageFlagBits Usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	bool bDeviceLocal = false;
};

class FBuffer : public FBufferBase
{
	TYPED_CLASS1(FBufferBase)
public:
	~FBuffer();
	void SetProperties(const FBufferInfo& InBufferInfo);
	void SetData(uint16_t InSize, const void* Data);
	void Init(uint32_t InSize);
	template<typename T>
	void SetData(const std::vector<T>& Data)
	{
		if (BufferInfo.bDeviceLocal)
		{
			MapMemoryDeviceLocal(Data.size() * sizeof(T), Data.data());
		}
		else
		{
			MapMemoryHostVisible(Data.size() * sizeof(T), Data.data());
		}
	}

	VkBuffer* GetBuffer();
	uint16_t GetSize();
	const FBufferInfo& GetBufferInfo();
	vk::DescriptorBufferInfo* GetDescriptorBufferInfo();
	void* MapData();
	void UnmapData();
private:
	void Destroy();
	void MapMemoryHostVisible(uint32_t InSize, const void* Data);
	void MapMemoryDeviceLocal(uint32_t InSize, const void* Data);
	void SizeUpdated();
	void UpdateDescriptorBufferInfo();
	
	uint32_t Size = 0;
	VkBuffer Buffer = nullptr;
	VmaAllocation Allocation = nullptr;
	FBufferInfo BufferInfo;
	vk::DescriptorBufferInfo DescriptorBufferInfo;
};



