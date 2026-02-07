#pragma once
#include <boost/signals2.hpp>
#include <MyRTTI.h>

class FBufferBase
{
	TYPED_CLASS
public:
	bool IsInitialized();
	boost::signals2::signal<void(FBufferBase* Buffer)> OnSizeUpdated;
protected:
	bool bInitialized = false;
};