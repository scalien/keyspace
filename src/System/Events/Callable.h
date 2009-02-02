#ifndef CALLABLE_H
#define CALLABLE_H

class Callable
{
public:
    virtual void Execute() = 0;
};

template<class T>
class MFunc : public Callable
{
public:
	typedef void (T::*Callback)();
	
	T*			object;
	Callback	callback;
	
	MFunc(T* object_, Callback callback_) { object = object_; callback = callback_; }
	
	void Execute() { (object->*callback)(); }
};

class CFunc : public Callable
{
public:
	typedef void (*Callback)();
	
	Callback	callback;
	
	CFunc(Callback callback_) { callback = callback_; }
	
	void Execute() { (*callback)(); }
};

inline void Call(Callable* callable) { if (callable) callable->Execute(); }

#endif
