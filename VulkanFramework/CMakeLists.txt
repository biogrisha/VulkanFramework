find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(Vulkan REQUIRED)
find_package(Freetype REQUIRED)
find_package(Boost REQUIRED COMPONENTS signals2)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Stb REQUIRED)

# Create a library target
add_library(VulkanFramework STATIC 
	include/DescriptorManager.h
	include/DescriptorSetLayout.h
	include/Pipeline.h
	include/VertexInputLayout.h
	include/VulkanHelpers.h
	include/vk_mem_alloc.h
	include/VmaUsage.h
	include/VulkanContext.h
	include/BufferBase.h
	include/Buffer.h
	include/ImageBuffer.h
	include/Rendering.h
	include/Atlas.h
	src/DescriptorManager.cpp
	src/DescriptorSetLayout.cpp
	src/Pipeline.cpp
	src/VertexInputLayout.cpp
	src/VulkanHelpers.cpp
	src/VmaUsage.cpp
	src/VulkanContext.cpp
	src/BufferBase.cpp
	src/Buffer.cpp
	src/ImageBuffer.cpp
	src/Rendering.cpp
	src/Atlas.cpp
)

# Set include directories
target_include_directories(VulkanFramework
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
		${Stb_INCLUDE_DIR}
)

target_link_libraries(VulkanFramework
		PRIVATE
		glfw
		glm::glm
		Vulkan::Vulkan
		Freetype::Freetype
		PUBLIC
		MyRTTI
		nlohmann_json::nlohmann_json
)

