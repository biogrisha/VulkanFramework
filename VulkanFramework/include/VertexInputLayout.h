#pragma once
#include <vulkan/vulkan_raii.hpp>

class FVertexInputLayout 
{
public:
	virtual std::vector<vk::VertexInputBindingDescription> getBindingDescription() = 0;
	virtual std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions() = 0;
};