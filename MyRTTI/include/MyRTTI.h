#pragma once
#include <memory>
#include <set>

#define TYPED_CLASS_COMMON \
protected:\
template<typename Type, typename ... Args>\
friend std::shared_ptr<Type> MyRTTI::MakeTypedShared(Args&& ... args);\
template<typename Type, typename ... Args>\
friend std::unique_ptr<Type> MyRTTI::MakeTypedUnique(Args&& ... args);\
template<typename To, typename From>\
friend To* MyRTTI::Cast(From* a);\
template<typename To, typename From>\
friend To* MyRTTI::Cast(const std::shared_ptr<From>& a);\
inline static MyRTTI::FTypeInfo TypeInfo;\
virtual MyRTTI::FTypeInfo* GetTypeInfo(){\
if(TypeInfo.TypeSet.empty()){TypeInfo.TypeSet = GetTypes();}\
return &TypeInfo;}\
static std::set<uint16_t> GetTypes()

#define TYPED_CLASS \
TYPED_CLASS_COMMON \
{\
	std::set<uint16_t> Types;\
	Types.insert(TypeInfo.Type);\
	return Types;\
}
#define TYPED_CLASS1(BaseClass) \
TYPED_CLASS_COMMON \
{\
	auto Types = BaseClass::GetTypes();\
	Types.insert(TypeInfo.Type);\
	return Types;\
}
#define TYPED_CLASS2(BaseClass1, BaseClass2) \
TYPED_CLASS_COMMON \
{\
	auto Types1 = BaseClass1::GetTypes();\
	auto Types2 = BaseClass2::GetTypes();\
	Types1.insert(Types2.begin(), Types2.end());\
	Types1.insert(TypeInfo.Type);\
	return Types1;\
}
#define TYPED_CLASS3(BaseClass1, BaseClass2, BaseClass3) \
TYPED_CLASS_COMMON \
{\
auto Types1 = BaseClass1::GetTypes();\
auto Types2 = BaseClass2::GetTypes();\
auto Types3 = BaseClass2::GetTypes();\
Types1.insert(Types2.begin(), Types2.end());\
Types1.insert(Types3.begin(), Types3.end());\
Types1.insert(TypeInfo.Type.Type);\
return Types1;\
}

namespace MyRTTI
{
	struct FTypeInfo
	{
		FTypeInfo()
		{
			Type = LastType;
			++LastType;
		}

		int Type = 0;
		std::set<uint16_t> TypeSet;
		inline static int LastType = 0;
	};

	template <typename Type, typename... Args>
	std::shared_ptr<Type> MakeTypedShared(Args&&... args)
	{
		if (Type::TypeInfo.TypeSet.empty())
		{
			Type::TypeInfo.TypeSet = Type::GetTypes();
		}
		std::shared_ptr<Type> SharedThis = std::make_shared<Type>(std::forward<Args>(args)...);
		return SharedThis;
	}

	template <typename Type, typename... Args>
	std::unique_ptr<Type> MakeTypedUnique(Args&&... args)
	{
		if (Type::TypeInfo.TypeSet.empty())
		{
			Type::TypeInfo.TypeSet = Type::GetTypes();
		}
		std::unique_ptr<Type> SharedThis = std::make_unique<Type>(std::forward<Args>(args)...);
		return SharedThis;
	}

	template <typename To, typename From>
	To* Cast(From* a)
	{
		if (!a)
		{
			return nullptr;
		}

		std::set<int> TypeSet;
		if (a->GetTypeInfo()->TypeSet.contains(To::TypeInfo.Type))
		{
			return static_cast<To*>(a);
		}
		return nullptr;
	}

	template <typename To, typename From>
	To* Cast(const std::shared_ptr<From>& a)
	{
		return Cast<To>(a.get());
	}

	class FSelfWeakable;

	template <typename T>
	class TTypedWeak
	{
	public:
		TTypedWeak() = default;

		TTypedWeak(const std::weak_ptr<FSelfWeakable>& InObjectWeak)
		{
			ObjectWeak = InObjectWeak;
			ObjectPtr = ObjectWeak.lock().get();
		}

		template <class Ty2>
		TTypedWeak(const TTypedWeak<Ty2>& Other) requires std::_SP_pointer_compatible<Ty2, T>::value
		{
			if (Other.Get())
			{
				ObjectWeak = Other.GetInnerWeak();
				ObjectPtr = ObjectWeak.lock().get();
			}
		}

		bool operator==(const TTypedWeak&) const;
		T* operator->() const;
		T* Get() const;
		const std::weak_ptr<FSelfWeakable>& GetInnerWeak() const;
		bool IsValid() const;

	private:
		std::weak_ptr<FSelfWeakable> ObjectWeak;
		FSelfWeakable* ObjectPtr = nullptr;
	};

	template <typename T>
	bool TTypedWeak<T>::operator==(const TTypedWeak& Right) const
	{
		return ObjectPtr == Right.ObjectPtr;
	}

	template <typename T>
	T* TTypedWeak<T>::operator->() const
	{
		if (!ObjectWeak.expired())
		{
			return static_cast<T*>(ObjectPtr);
		}
		return nullptr;
	}

	template <typename T>
	T* TTypedWeak<T>::Get() const
	{
		if (!ObjectWeak.expired())
		{
			return static_cast<T*>(ObjectPtr);
		}
		return nullptr;
	}

	template <typename T>
	const std::weak_ptr<FSelfWeakable>& TTypedWeak<T>::GetInnerWeak() const
	{
		return ObjectWeak;
	}

	template <typename T>
	bool TTypedWeak<T>::IsValid() const
	{
		return !ObjectWeak.expired();
	}


	class FSelfWeakable
	{
		TYPED_CLASS
		FSelfWeakable()
		{
			SelfShared = std::shared_ptr<FSelfWeakable>(this, [](FSelfWeakable* obj)
				{
				});
		}
	private:
		template <typename T>
		friend TTypedWeak<T> GetTypedWeak(const std::shared_ptr<T>& Obj);
		template <typename T>
		friend TTypedWeak<T> GetTypedWeak(T* Obj);

		std::shared_ptr<FSelfWeakable> SelfShared;
	};

	template <typename T>
	TTypedWeak<T> GetTypedWeak(const std::shared_ptr<T>& Obj)
	{
		if (auto Weakable = Cast<FSelfWeakable>(Obj))
		{
			return TTypedWeak<T>(std::weak_ptr(Weakable->SelfShared));
		}
		return TTypedWeak<T>();
	}

	template <typename T>
	TTypedWeak<T> GetTypedWeak(T* Obj)
	{
		if (auto Weakable = Cast<FSelfWeakable>(Obj))
		{
			return TTypedWeak<T>(std::weak_ptr(Weakable->SelfShared));
		}
		return TTypedWeak<T>();
	}

}