#ifndef QUEUE_H
#define QUEUE_H

template<class T, T* T::*pnext>
class Queue
{
public:

	Queue()
	{
		size = 0;
		head = 0;
		tail = 0;
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
		assert(elem != 0);
		
		elem->*pnext = 0;
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
				tail = 0;
			elem->*pnext = 0;
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
