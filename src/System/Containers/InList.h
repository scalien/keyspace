#ifndef IN_LIST_H
#define IN_LIST_H

// Intrusive List template types

template<class T> class InListNode;
template<class T, InListNode<T> T::*pnode> class InList;


template<class T>
class ILNode
{
public:
	ILNode*		next;
	ILNode*		prev;
	T*			owner;
	
	ILNode()
	{
		next = prev = this;
		owner = 0;
	}
	
	~ILNode()
	{
		Remove();
	}
	
	bool IsLinked()
	{
		return owner ? true : false;
	}
	
	void Remove()
	{
		next->prev = prev;
		prev->next = next;
		next = this;
		prev = this;
		owner = 0;
	}
};

template<class T, ILNode<T> T::*pnode>
class IList
{
public:
	ILNode<T>*	head;
	ILNode<T>*	tail;
	int			size;

	IList()
	{
		head = tail = 0;
		size = 0;
	}
	
	T* Next(T* t)
	{
		ILNode<T>* node = t->*pnode.next;
		if (node)
			return node->owner;
		return 0;
	}
	
	T* Prev(T* t)
	{
		ILNode<T>* node = t->*pnode.prev;
		if (node)
			return node->owner;
		return 0;
	}

	void Add(T &t)
	{
		ILNode<T>* node = &(t.*pnode);

		node->next = head;
		node->prev = 0;
		
		if (head)
			head->prev = node;
		head = node;
		size++;
		
		if (!tail)
			tail = node;
		
		node->owner = &t;
	}
	
	void Append(T &t)
	{
		ILNode<T>* node = &(t->*pnode);

		node->next = 0;
		node->prev = tail;
		
		if (tail)
			tail->next = node;
		tail = node;
		size++;
		
		if (!head)
			head = node;
		
		node->owner = &t;
	}
	
	T* Remove(T* t)
	{
		ILNode<T>* node = &(t->*pnode);
		T* ret;

		if (head == node)
			head = node;
		else
			node->prev->next = node;
		
		if (tail == node)
			tail = Prev(t);
		else
			node->next->prev = node;
		
		size--;
		node->owner = 0;
		ret = 0;
		if (Next(t))
			ret = t;
		return ret;
	}
};

template<class T>
class InListNode
{
public:
	T*	next;
	T*	prev;

	InListNode()
	{
		next = prev = 0;
	}	
};

template<class T, InListNode<T> T::*pnode>
class InList
{
public:
	T*		head;
	T*		tail;
	int		size;
	
	InList()
	{
		head = tail = 0;
		size = 0;
	}
	
	int Size() { return size; }
	
	T* Head()
	{
		return head;
	}
	
	T* Tail()
	{
		return tail;
	}
	
	T* Next(T* t)
	{
		return (t->*pnode).next;
	}
	
	T* Prev(T* t)
	{
		return (t->*pnode).prev;
	}
	
	void Add(T &t)
	{
		(t.*pnode).next = head;
		(t.*pnode).prev = 0;
		
		if (head != 0)
			(head->*pnode).prev = &t;
		head = &t;
		size++;
		
		if (tail == 0)
			tail = &t;
	}
	
	void Append(T &t)
	{
		(t.*pnode).next = 0;
		(t.*pnode).prev = tail;
		
		if (tail != 0)
			(tail->*pnode).next = &t;
		tail = &t;
		size++;
		
		if (head == 0)
			head = &t;
	}
	
	T* Remove(T* t)
	{
		T* ret;
		
		if (head == t)
			head = Next(t);
		else
			(Prev(t)->*pnode).next = t;
		
		if (tail == t)
			tail = Prev(t);
		else
			(Next(t)->*pnode).prev = t;
		
		size--;
		ret = 0;
		if (Next(t))
			ret = Next(t);
			
		return ret;
	}
};

template<class T, T* T::*pnext, T* T::*pprev>
class InList2
{
public:
	T*		head;
	T*		tail;
	int		size;

	InList2()
	{
		head = tail = 0;
		size = 0;
	}

	int Size() { return size; }
	
	T* Head()
	{
		return head;
	}
	
	T* Tail()
	{
		return tail;
	}
	
	T* Next(T* t) 
	{
		return t->*pnext;
	}

	T* Prev(T* t)
	{
		return t->*pprev;
	}
	
	void Add(T &t)
	{
		t.*pnext = head;
		t.*pprev = 0;
		
		if (head != 0)
			head->*pprev = &t;
		head = &t;
		size++;
		
		if (tail == 0)
			tail = &t;
	}
	
	void Append(T &t)
	{
		t.*pnext = 0;
		t.*pprev = tail;
		
		if (tail != 0)
			tail->*pnext = &t;
		tail = &t;
		size++;
		
		if (head == 0)
			head = &t;
	}
	
	T* Remove(T* t)
	{
		T* ret;
		
		if (head == t)
			head = Next(t);
		else
			t->*pprev->*pnext = t->*pnext;
			
		if (tail == t)
			tail = Prev(t);
		else
			t->*pnext->*pprev = t->*pprev;
		
		size--;
		ret = 0;
		if (t->*pnext)
			ret = Next(t);
			
		return ret;
	}
};

#endif
