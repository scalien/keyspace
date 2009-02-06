#ifndef LIST_H
#define LIST_H

#include <stddef.h>

template<class T>
struct ListNode
{
	T           	data;
	
	ListNode<T>*	next;
	ListNode<T>*	prev;
	
	void Init(T data_, ListNode<T>* next_, ListNode<T>* prev_)
	{
		data = data_;
		next = next_;
		prev = prev_;
	}
};

template<class T>
class List
{
public:
	List();    
	~List();
    
	int				Size();
	
    T*				Head();
	T*				Tail();
    
    T*				Next(T* t);
    T*				Prev(T* t);
    
    virtual void	Add(T t);
    //virtual void	AddUnique(T t);

	T*				Remove(T* t);
	bool			Remove(T t);

protected:
	ListNode<T>*	head;
	ListNode<T>*	tail;
    int				size;
};

template<class T>
List<T>::List()
{
	head = NULL;
	tail = NULL;
	size = 0;
}

template<class T>
List<T>::~List()
{
	ListNode<T> *t, *n;
	
	t = head;
	while (t)
	{
		n = t->next;
		delete t;
		t = n;
		size--;
	}
}

template<class T>
int List<T>::Size()
{
	return size;
}

template<class T>
T* List<T>::Head()
{
	if (head == NULL)
		return NULL;
	else
		return &head->data;
}

template<class T>
T* List<T>::Tail()
{
	if (tail == NULL)
		return NULL;
	else
		return &tail->data;
}

template<class T>
T* List<T>::Next(T* t)
{
	ListNode<T>* node = (ListNode<T>*) t;
	if (node->next == NULL)
		return NULL;
	else
		return &node->next->data;
}

template<class T>
T* List<T>::Prev(T* t)
{
	ListNode<T>* node = (ListNode<T>*) t;
	if (node->prev == NULL)
		return NULL;
	else
		return &node->prev->data;
}

template<class T>
void List<T>::Add(T t)
{
	ListNode<T>* node;

	node = new ListNode<T>;
	node->Init(t, head, NULL);
	
	if (head != NULL)
		head->prev = node;
	head = node;
	size++;

	if (tail == NULL)
		tail = node;
}

/*
template<class T>
virtual void List<T>::AddUnique(T t)
{
	T* it;
	for (it = Head(); it != NULL; it = Next(it))
	{
		if (*it == t) // unique-check
			return;
	}
	
	Add(t);
}
*/

template<class T>
T* List<T>::Remove(T* t)
{
	ListNode<T>* node;
	T* ret;
	
	node = (ListNode<T>*) t;
	if (head == node)
		head = node->next;
	else
		node->prev->next = node->next;
	
	if (tail == node)
		tail = node->prev;
	else
		node->next->prev = node->prev;
		
	size--;
	
	ret = NULL;
	if (node->next != NULL)
		ret = &node->next->data;
	
	delete node;
	return ret;
}

template<class T>
bool List<T>::Remove(T t)
{
	T* it;
	
	for (it = Head(); it != NULL; it = Next(it))
	{
		if (*it == t)
		{
			Remove(it);
			return true;
		}
	}
	
	// not found
	return false;
}

#endif
