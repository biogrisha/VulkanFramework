#pragma once
#include <vulkan/vulkan_raii.hpp>

class DescriptorSetLayout 
{
public:
	void Setup(const vk::raii::Device& device, const std::vector<vk::DescriptorSetLayoutBinding>& bindings);
	vk::DescriptorSetLayout GetDescriptorSetLayout() const;
private:
	vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
};
