#ifndef LIST_H
#define LIST_H

#include <stddef.h>

template<class T>
struct ListNode
{
	T           	data;
	
	ListNode<T>*	next;
	ListNode<T>*	prev;
	
	void Init(T &data_, ListNode<T>* next_, ListNode<T>* prev_)
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
		ListNode<T>*	head;
		ListNode<T>*	tail;
		int				length;
		
		List() { head = NULL; tail = NULL; length = 0; }
		
		virtual ~List()
		{
			ListNode<T> *t, *n;
			
			t = head;
			while (t)
			{
				n = t->next;
				delete t;
				t = n;
				length--;
			}
		}
		
		int Length()
		{
			return length;
		}
		
		T* Head()
		{
			if (head == NULL)
				return NULL;
			else
				return &head->data;
		}
		
		T* Tail()
		{
			if (tail == NULL)
				return NULL;
			else
				return &tail->data;
		}
		
		T* Next(T* t)
		{
			ListNode<T>* node = (ListNode<T>*) t;
			if (node->next == NULL)
				return NULL;
			else
				return &node->next->data;
		}
		
		T* Prev(T* t)
		{
			ListNode<T>* node = (ListNode<T>*) t;
			if (node->prev == NULL)
				return NULL;
			else
				return &node->prev->data;
		}
		
		virtual void Add(T &t)
		{
			ListNode<T>* node;
			
			node = new ListNode<T>;
			node->Init(t, head, NULL);
			
			if (head != NULL)
				head->prev = node;
			head = node;
			length++;
			
			if (tail == NULL)
				tail = node;
		}
		
		void Append(T &t)
		{
			ListNode<T>* node;
			
			node = new ListNode<T>;
			node->Init(t, NULL, tail);
			
			if (tail != NULL)
				tail->next = node;
			tail = node;
			length++;
			
			if (head == NULL)
				head = node;
		}
		
		T* Remove(T* t)
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
			
			length--;
			
			ret = NULL;
			if (node->next != NULL)
				ret = &node->next->data;
			
			delete node;
			return ret;
		}
		
		bool Remove(T &t)
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
	};

#endif
