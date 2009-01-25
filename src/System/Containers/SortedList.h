#ifndef SORTEDLIST_H
#define SORTEDLIST_H

#include "List.h"

template<class T>
class SortedList : public List<T>
{
public:
    void Add(T t)
    {
		Add(t, false);
    }
	
    /*
    void AddUnique(T t)
	{
		Add(t, true);
	}
	*/
	
private:
    void Add(T t, bool unique)
	{
        ListNode<T>*  node;
        ListNode<T>** curr = &this->head;

		while(true) {
        	if (*curr == NULL || LessThan(t, (*curr)->data)) {
	            node = new ListNode<T>;
				node->data = t;
				node->next = *curr;
				if (curr != &this->head)
					node->prev = (ListNode<T>*) ((char*)curr - offsetof(ListNode<T>, next));
				else
					node->prev = NULL;
				if (*curr != NULL)
					(*curr)->prev = node;
				if (*curr == NULL)
					this->tail = node;
				*curr = node;
				this->size++;		
				return;
	        } else {
	            if (unique) {
					if ((*curr)->data == t) // unique-check
	                	return;
				}
	            curr = &(*curr)->next;
	        }
		}
    }
};

#endif