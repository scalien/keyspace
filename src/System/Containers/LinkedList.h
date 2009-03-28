#ifndef LINKED_LIST_H
#define LINKED_LIST_H

template<class T> class LinkedListNode;
template<class T, LinkedListNode<T> T::*pnode> class LinkedList;


template<class T>
class LLNode
{
public:
	LLNode*		next;
	LLNode*		prev;
	T*			owner;
	
	LLNode()
	{
		next = prev = NULL;
		owner = NULL;
	}
	
	~LLNode()
	{
		Remove();
	}
	
	void Remove()
	{
		next->prev = prev;
		prev->next = next;
		next = this;
		prev = this;
	}
};

template<class T, LLNode<T> T::*pnode>
class LList
{
public:
	LLNode<T>*	head;
	LLNode<T>*	tail;
	int			size;

	LList()
	{
		head = tail = NULL;
		size = 0;
	}
	
	T* Next(T* t)
	{
		LLNode<T>* node;
		node = &(t->*pnode);
		if (node)
			return node->owner;
		return NULL;
	}

	void Add(T &t)
	{
		(t.*pnode).next = head;
		(t.*pnode).prev = NULL;
		
		if (head != NULL)
			head->prev = &(t.*pnode);
		head = &(t.*pnode);
		size++;
		
		if (tail == NULL)
			tail = &(t.*pnode);
		
		(t.*pnode).owner = &t;
	}
};

template<class T>
class LinkedListNode
{
public:
	T*	next;
	T*	prev;

	LinkedListNode()
	{
		next = prev = NULL;
	}	
};

template<class T, LinkedListNode<T> T::*pnode>
class LinkedList
{
public:
	T*		head;
	T*		tail;
	int		size;
	
	LinkedList()
	{
		head = tail = NULL;
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
		(t.*pnode).prev = NULL;
		
		if (head != NULL)
			(head->*pnode).prev = &t;
		head = &t;
		size++;
		
		if (tail == NULL)
			tail = &t;
	}
	
	void Append(T &t)
	{
		(t.*pnode).next = NULL;
		(t.*pnode).prev = tail;
		
		if (tail != NULL)
			(tail->*pnode).next = &t;
		tail = &t;
		size++;
		
		if (head == NULL)
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
		ret = NULL;
		if (Next(t))
			ret = Next(t);
			
		return ret;
	}
};

template<class T, T* T::*pnext, T* T::*pprev>
class LinkedList2
{
public:
	T*		head;
	T*		tail;
	int		size;

	LinkedList2()
	{
		head = tail = NULL;
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
		t.*pprev = NULL;
		
		if (head != NULL)
			head->*pprev = &t;
		head = &t;
		size++;
		
		if (tail == NULL)
			tail = &t;
	}
	
	void Append(T &t)
	{
		t.*pnext = NULL;
		t.*pprev = tail;
		
		if (tail != NULL)
			tail->*pnext = &t;
		tail = &t;
		size++;
		
		if (head == NULL)
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
		ret = NULL;
		if (t->*pnext)
			ret = Next(t);
			
		return ret;
	}
};

#endif
