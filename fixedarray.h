#pragma once
#include "podhelp.h"
#include "debug.h"

//POD only
template< typename T, uint32_t SIZE, bool bISPOD = true>
struct TFixedArray
{
	typedef PODHelp<T,bISPOD> POD; 
	union
	{
		char buffer [sizeof(T)* SIZE];
		uint64_t foo;//force alignment to 8byte: todo make it inherit from type
	};

	TFixedArray()
	:nSize(0)
	{}	
	uint32_t 	nSize;

	//T* Ptr(){ return &Array[0]; }
	T* Ptr(){ return (T*)&buffer[0]; }
	const T* Ptr() const { return (const T*)&buffer[0]; }
	uint32_t Size(){ return nSize; }
	uint32_t Capacity(){return SIZE;}
	bool Full(){ return Size() == Capacity(); }

	T* PushBack()
	{ 
		ZASSERT(nSize<SIZE); 
		POD::Construct(&Ptr()[nSize]);
		return &Ptr()[nSize++];
	}
	T* PushBack(T* p, uint32_t nCount)
	{
		ZASSERT(nCount + nSize < Capacity());
		for(uint32_t i = 0; i < nCount; ++i)
		{
			POD::Construct(&Ptr()[i+nSize]);
			Ptr()[i+nSize] = p[i];
		}
		nSize += nCount;
		return &Ptr()[nSize-nCount];
	}
	void PushBack(const T& value)
	{ 
		ZASSERT(nSize<SIZE);
		POD::Construct(&Ptr()[nSize]);
		Ptr()[nSize++] = value;
	}
	T PopBack()
	{
		ZASSERT(nSize);
		T Back = Ptr()[--nSize];
		POD::Destruct(&Ptr()[nSize]);
		return Back;
	}
	T Top()
	{
		ZASSERT(nSize);
		return Ptr()[nSize-1];
	}
	T* TopPtr()
	{
		ZASSERT(nSize);
		return &Ptr()[nSize-1];
	}	
	bool Empty(){return 0 == nSize; }
	void Clear()
	{
		if(!bISPOD)
		{
			for(uint32_t i = 0; i < nSize; ++i)
			{
				POD::Destruct(&Ptr()[i]);
			}
		}
		nSize = 0; 
	}

	void EraseSwapLast(uint32_t nIndex)
	{
		--nSize;
		Ptr()[nIndex] = Ptr()[nSize];
		POD::Destruct(&Ptr()[nSize]);
	}
	void Resize(uint32_t nSize_)
	{
		if(!bISPOD)
		{
			if(nSize < nSize_)
				for(uint32_t i = nSize; i < nSize_; ++i)
					POD::Construct(&Ptr()[i]);
			else
				for(uint32_t i = nSize_; i < nSize; ++i)
					POD::Destruct(&Ptr()[i]);
		}
		nSize = nSize_;
	}

	T* Begin(){ return Ptr();}
	T* End(){ return Ptr() + nSize; }
	const T* Begin() const { return Ptr();}
	const T* End() const { return Ptr() + nSize; }
	T* begin(){ return Ptr();}
	T* end(){ return Ptr() + nSize; }
	const T* begin() const { return Ptr();}
	const T* end() const { return Ptr() + nSize; }

	T& operator[](const size_t nIndex){ ZASSERT(nIndex < nSize); return Ptr()[nIndex]; }
	const T& operator[](const size_t nIndex) const{ ZASSERT(nIndex < nSize); return Ptr()[nIndex]; }

};
