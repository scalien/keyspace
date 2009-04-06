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
		next = prev = this;
		owner = 0;
	}
	
	~LLNode()
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

template<class T, LLNode<T> T::*pnode>
class LList
{
public:
	LLNode<T>*	head;
	LLNode<T>*	tail;
	int			size;

	LList()
	{
		head = tail = 0;
		size = 0;
	}
	
	T* Next(T* t)
	{
		LLNode<T>* node = t->*pnode.next;
		if (node)
			return node->owner;
		return 0;
	}
	
	T* Prev(T* t)
	{
		LLNode<T>* node = t->*pnode.prev;
		if (node)
			return node->owner;
		return 0;
	}

	void Add(T &t)
	{
		LLNode<T>* node = &(t->*pnode);

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
		LLNode<T>* node = &(t->*pnode);

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
		LLNode<T>* node = &(t->*pnode);
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
class LinkedListNode
{
public:
	T*	next;
	T*	prev;

	LinkedListNode()
	{
		next = prev = 0;
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
class LinkedList2
{
public:
	T*		head;
	T*		tail;
	int		size;

	LinkedList2()
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
