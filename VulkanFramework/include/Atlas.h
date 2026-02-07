#pragma once
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include "ImageBuffer.h"
#include <string>
#include <fstream>
#include <nlohmann/json.hpp>

struct FSpriteInfo
{
	glm::vec2 TexPos;
	glm::vec2 TexSize;
};

class FAtlas
{
public:
	FAtlas();
	void SetPath(const std::string& AtlasPath);
	void Init();
	FImageBuffer* GetImage();
	FSpriteInfo GetInfo(const std::string& Name);
private:
	std::string Path;
	std::unique_ptr<FImageBuffer> Image;
	nlohmann::json MetaData;
};