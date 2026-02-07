#pragma once
#include <vector>
#include <memory>
#include <Buffer.h>

class FDescriptorSetLayoutV2
{
public:
	std::vector<std::unique_ptr<FDescriptorSetV2>> DescriptorSetDatas;
};
class FDescriptorV2
{
	FBufferV2* Buffer = nullptr;
};
class FDescriptorSetV2
{
public:
	bool bBusy = false;
	FDescriptorSetLayoutV2* Layout = nullptr;
	std::vector<FDescriptorV2> Descriptors;
};
class FDescriptorManagerV2
{
public:
	FDescriptorSetV2* GetOrCreateDescriptorSet(const std::vector<FBufferV2*>& Buffers);
	FDescriptorSetV2* FindDescriptorSet(const std::vector<FBufferV2*>& Buffers);
	std::vector<std::unique_ptr<FDescriptorSetLayoutV2>> Layouts;
};