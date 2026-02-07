#include "Buffer.h"
#include <VulkanHelpers.h>
#include <VulkanContext.h>
#include <boost/signals2.hpp>


FBuffer::~FBuffer()
{
	Destroy();
}

void FBuffer::SetProperties(const FBufferInfo& InBufferInfo)
{
	BufferInfo = InBufferInfo;
}

void FBuffer::SetData(uint16_t InSize, const void* Data)
{
	bInitialized = true;
	if (BufferInfo.bDeviceLocal)
	{
		MapMemoryDeviceLocal(InSize, Data);
	}
	else
	{
		MapMemoryHostVisible(InSize, Data);
	}
}

void FBuffer::Init(uint32_t InSize)
{
	Size = InSize;
	VkBufferCreateInfo bufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufCreateInfo.size = InSize;
	bufCreateInfo.usage = BufferInfo.Usage;
	bufCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo AllocInfo = {};
	AllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	if (BufferInfo.bDeviceLocal)
	{
		AllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	}
	else
	{
		AllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	}
	auto Res = vmaCreateBuffer(FVulkanStatic::Context->VmaAllocator, &bufCreateInfo, &AllocInfo, &Buffer, &Allocation, nullptr);
	SizeUpdated();
}

VkBuffer* FBuffer::GetBuffer()
{
	return &Buffer;
}

uint16_t FBuffer::GetSize()
{
	return Size;
}

const FBufferInfo& FBuffer::GetBufferInfo()
{
	return BufferInfo;
}

vk::DescriptorBufferInfo* FBuffer::GetDescriptorBufferInfo()
{
	return &DescriptorBufferInfo;
}

void* FBuffer::MapData()
{
	void* mappedData;
	vmaMapMemory(FVulkanStatic::Context->VmaAllocator, Allocation, &mappedData);
	return mappedData;
}

void FBuffer::UnmapData()
{
	vmaUnmapMemory(FVulkanStatic::Context->VmaAllocator, Allocation);
}

void FBuffer::Destroy()
{
	if (Size > 0)
	{
		vmaDestroyBuffer(FVulkanStatic::Context->VmaAllocator, Buffer, Allocation);
	}
}

void FBuffer::MapMemoryHostVisible(uint32_t InSize, const void* Data)
{
	if (InSize > Size)
	{
		//if requires more memory-> reallocate the buffer
		//New buffer create info
		VkBufferCreateInfo bufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufCreateInfo.size = InSize;
		bufCreateInfo.usage = BufferInfo.Usage;
		bufCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		//New buffer alloc info
		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

		//creating a new buffer
		VkBuffer buf;
		VmaAllocation alloc;
		vmaCreateBuffer(FVulkanStatic::Context->VmaAllocator, &bufCreateInfo, &allocCreateInfo, &buf, &alloc, nullptr);
		
		auto OldBuffer = Buffer;
		auto OldAllocation = Allocation;
		auto OldSize = Size;

		Size = InSize;
		Allocation = alloc;
		Buffer = buf;
		SizeUpdated();

		//Destroy old buffer only after OnSizeUpdated callback
		if (OldSize != 0)
		{
			//if this is not the first allocation -> deallocate old buffer
			vmaDestroyBuffer(FVulkanStatic::Context->VmaAllocator, OldBuffer, OldAllocation);
		}
	}
	//write into buffer
	vmaCopyMemoryToAllocation(FVulkanStatic::Context->VmaAllocator, Data, Allocation, 0, InSize);
}

void FBuffer::MapMemoryDeviceLocal(uint32_t InSize, const void* Data)
{
	if (InSize > Size)
	{
		//If size required become larger
		//reallocate buffer
		VkBufferCreateInfo bufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufCreateInfo.size = InSize;
		bufCreateInfo.usage = BufferInfo.Usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		bufCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		//Hint vma to create device local memory
		VmaAllocationCreateInfo AllocInfo = {};
		AllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		AllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		//create new device local buffer
		VkBuffer buf;
		VmaAllocation alloc;
		auto Res = vmaCreateBuffer(FVulkanStatic::Context->VmaAllocator, &bufCreateInfo, &AllocInfo, &buf, &alloc, nullptr);
		switch (Res) {
		case VK_SUCCESS:
			// ok
			break;

		case VK_ERROR_OUT_OF_HOST_MEMORY:
			std::cerr << "VMA: Out of RAM\n";
			break;

		case VK_ERROR_OUT_OF_DEVICE_MEMORY:
			std::cerr << "VMA: Out of VRAM\n";
			break;

		default:
			std::cerr << "VMA: Unknown error: " << Res << "\n";
			break;
		}

		auto OldSize = Size;
		auto OldBuffer = Buffer;
		auto OldAllocation = Allocation;

		Size = InSize;
		Allocation = alloc;
		Buffer = buf;
		SizeUpdated();
		
		//Destroy old buffer only after OnSizeUpdated callback
		if (OldSize != 0)
		{
			//if this is not the first allocation -> deallocate old buffer
			vmaDestroyBuffer(FVulkanStatic::Context->VmaAllocator, OldBuffer, OldAllocation);
		}
	}
	//Create a staging buffer with host access
	VkBufferCreateInfo StagingBufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	StagingBufferCreateInfo.size = InSize;
	StagingBufferCreateInfo.usage = BufferInfo.Usage | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	StagingBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo StagingAllocCreateInfo = {};
	StagingAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
	StagingAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

	VkBuffer StagingBuffer;
	VmaAllocation StagingAlloc;
	auto Res = vmaCreateBuffer(FVulkanStatic::Context->VmaAllocator, &StagingBufferCreateInfo, &StagingAllocCreateInfo, &StagingBuffer, &StagingAlloc, nullptr);
	switch (Res) {
	case VK_SUCCESS:
		// ok
		break;

	case VK_ERROR_OUT_OF_HOST_MEMORY:
		std::cerr << "VMA: Out of RAM\n";
		break;

	case VK_ERROR_OUT_OF_DEVICE_MEMORY:
		std::cerr << "VMA: Out of VRAM\n";
		break;

	default:
		std::cerr << "VMA: Unknown error: " << Res << "\n";
		break;
	}
	//write into staging buffer
	vmaCopyMemoryToAllocation(FVulkanStatic::Context->VmaAllocator, Data, StagingAlloc, 0, InSize);
	//copy data into device local buffer
	VkHelpers::CopyBufferRaw(&StagingBuffer, &Buffer, InSize);
	//destroy staging buffer
	vmaDestroyBuffer(FVulkanStatic::Context->VmaAllocator, StagingBuffer, StagingAlloc);
}

void FBuffer::SizeUpdated()
{
	UpdateDescriptorBufferInfo();
	OnSizeUpdated(this);
}

void FBuffer::UpdateDescriptorBufferInfo()
{
	DescriptorBufferInfo = vk::DescriptorBufferInfo(*GetBuffer(), 0, GetSize());
}

