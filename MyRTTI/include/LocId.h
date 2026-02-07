#pragma once

struct FLocId
{
	FLocId();
	FLocId(bool bInit);

	bool operator<(const FLocId& Other) const;
	bool operator==(const FLocId& Other) const;
private:
	unsigned int Id = 0;
	static inline unsigned int StaticId = 0;
};