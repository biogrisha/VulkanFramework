#include "LocId.h"

FLocId::FLocId()
{
	Id = StaticId;
	StaticId++;
}

FLocId::FLocId(bool bInit)
{
	if (bInit)
	{
		Id = StaticId;
		StaticId++;
	}
}

bool FLocId::operator<(const FLocId& Other) const
{
	return Id < Other.Id;
}

bool FLocId::operator==(const FLocId& Other) const
{
	return Id == Other.Id;
}
