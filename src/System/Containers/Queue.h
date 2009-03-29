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

	int Size() { return size; }
	
	void Clear()
	{
		T* elem;

		do
		{
			elem = Get();
		} while (elem);
	}
	
	void Append(T* elem)
	{
		elem->*pnext = NULL;
		if (tail)
			tail->*pnext = elem;
		else
			head = elem;
		tail = elem;
		size++;
	}
	
	T* Get()
	{
		T* elem;
		elem = head;
		if (elem)
		{
			head = elem->*pnext;
			if (tail == elem)
				tail = NULL;
			elem->*pnext = NULL;
			size--;
		}
		return elem;
	}
	
	T* Head()
	{
		return head;
	}
	
	T* Tail()
	{
		return tail;
	}

private:
	T*		head;
	T*		tail;
	int		size;
};


#endif
