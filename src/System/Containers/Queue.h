#ifndef QUEUE_H
#define QUEUE_H

template<class T, T* T::*pnext>
class Queue
{
public:

	Queue()
	{
		Clear();
	}

	bool IsEmpty()
	{
		if (first == last && first == NULL)
			return true;
		return false;
	}
	
	void Clear()
	{
		first = NULL;
		last = NULL;
	}
	
	void Add(T* elem)
	{
		elem->*pnext = NULL;
		if (last)
			last->*pnext = elem;
		else
			first = elem;
		last = elem;
	}
	
	T* Get()
	{
		T* elem;
		elem = first;
		if (elem)
		{
			first = elem->*pnext;
			if (last == elem)
				last = NULL;
			elem->*pnext = NULL;
		}
		return elem;
	}
	
	T* First()
	{
		return first;
	}
	
	T* Last()
	{
		return last;
	}

private:
	T*	first;
	T*	last;
};


#endif
