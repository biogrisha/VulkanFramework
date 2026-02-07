#include "Atlas.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using json = nlohmann::json;

FAtlas::FAtlas()
{

}

void FAtlas::SetPath(const std::string& AtlasPath)
{
	Path = AtlasPath;
}

void FAtlas::Init()
{
	int texWidth, texHeight, texChannels;
	std::string ImgPath = Path + ".png";
	stbi_uc* pixels = stbi_load(ImgPath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	Image = MyRTTI::MakeTypedUnique<FImageBuffer>();
	Image->SetExtent({uint32_t(texWidth), uint32_t(texHeight)});
	Image->AddUsageFlags(VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	Image->Init();
	Image->UpdateImageFromData(pixels);

	std::string MetaPath = Path + ".json";
	std::ifstream MetaDataFile(MetaPath);
	MetaDataFile >> MetaData;

}

FImageBuffer* FAtlas::GetImage()
{
	return Image.get();
}

FSpriteInfo FAtlas::GetInfo(const std::string& Name)
{
	auto Frames = MetaData["frames"];

	for (auto Frame : Frames)
	{
		auto Filename = Frame["filename"].get<std::string>();
		if (Name == Filename)
		{
			FSpriteInfo Result;
			auto SpriteJson = Frame["frame"];
			Result.TexPos.x = SpriteJson["x"].get<int>();
			Result.TexPos.y = SpriteJson["y"].get<int>();
			Result.TexSize.x = SpriteJson["w"].get<int>();
			Result.TexSize.y = SpriteJson["h"].get<int>();
			return Result;
		}
	}
	
	return FSpriteInfo();
}
