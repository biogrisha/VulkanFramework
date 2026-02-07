#include "DescriptorSetLayout.h"

void DescriptorSetLayout::Setup(const vk::raii::Device& device, const std::vector<vk::DescriptorSetLayoutBinding>& bindings)
{
	vk::DescriptorSetLayoutCreateInfo layoutInfo({}, bindings.size(), bindings.data());
	descriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);
}

vk::DescriptorSetLayout DescriptorSetLayout::GetDescriptorSetLayout() const
{
	return *descriptorSetLayout;
}
