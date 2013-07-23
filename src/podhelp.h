#pragma once

//nasty shit
template<typename T, bool POD = true>
struct PODHelp
{
	static inline void Construct(T* t){}
	static inline void Destruct(T* t){}
};
template<typename T>
struct PODHelp<T, false>
{
	static inline void Construct(T* t)
	{
		new (t) T();
	}
	static inline void Destruct(T* t)
	{
		t->~T();
	}
};


