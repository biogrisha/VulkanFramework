#include "ComponentInterface/ComponentInterface.h"


void FComponent::RemoveFromOwner()
{
	Owner->RemoveComponent(Id);
}

FComponentHost* FComponent::GetOwner()
{
	return Owner;
}

void FComponentHost::AddComponent(const FLocId& LocId, const std::shared_ptr<FComponent>& Component)
{
	Component->Owner = this;
	Component->Id = LocId;
	Components.insert_or_assign(LocId, Component);
}

void FComponentHost::RemoveComponent(const FLocId& LocId)
{
	Components.erase(LocId);
}

std::shared_ptr<FComponent> FComponentHost::GetComponent(const FLocId& LocId)
{
	if (auto It = Components.find(LocId); It != Components.end())
	{
		return It->second;
	}
	return nullptr;
}
