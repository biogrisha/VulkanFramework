#pragma once
#include <MyRTTI.h>
#include <map>
#include <LocId.h>

class FComponentHost;
class FComponent
{
	TYPED_CLASS
public:
	void RemoveFromOwner();
	FComponentHost* GetOwner();
private:
	friend class FComponentHost;
	FComponentHost* Owner = nullptr;
	FLocId Id = FLocId(false);
};

class FComponentHost
{
	TYPED_CLASS
public:
	void AddComponent(const FLocId& LocId, const std::shared_ptr<FComponent>& Component);
	void RemoveComponent(const FLocId& LocId);
	std::shared_ptr<FComponent> GetComponent(const FLocId& LocId);
private:
	std::map<FLocId, std::shared_ptr<FComponent>> Components;
};