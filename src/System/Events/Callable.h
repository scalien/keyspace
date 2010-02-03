#ifndef CALLABLE_H
#define CALLABLE_H

class Callable
{
public:
	virtual ~Callable() {}
    virtual void Execute() = 0;
};

template<class T>
class MFunc : public Callable
{
public:
	typedef void (T::*Callback)();
	
	T*			object;
	Callback	callback;
	
	MFunc(T* object_, Callback callback_)
	{
		object = object_;
		callback = callback_;
	}
	
	void Execute()
	{
		(object->*callback)();
	}
};

template<class T, class P>
class MFuncParam : public Callable
{
public:
	typedef void (T::*Callback)(P param);
	
	T*			object;
	Callback	callback;
	P			param;
	
	MFuncParam(T* object_, Callback callback_)
	{
		object = object_;
		callback = callback_;
	}
	
	void Execute()
	{
		(object->*callback)(param);
	}
};

class CFunc : public Callable
{
public:
	typedef void (*Callback)();
	
	Callback	callback;
	
	CFunc(Callback callback_)
	{
		callback = callback_;
	}
	
	CFunc()
	{
		callback = 0;
	}
	
	void Execute()
	{
		if (callback)
			(*callback)();
	}
};

inline void Call(Callable* callable)
{
	if (callable)
		callable->Execute();
}

template<class T, void (T::*Callback)()>
Callable* InitMFunc(T* t, Callable* callable)
{
	return new (callable) MFunc<T>(t, Callback);
}

#endif
