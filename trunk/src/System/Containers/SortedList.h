#ifndef SORTEDLIST_H
#define SORTEDLIST_H

#include "List.h"
#include "System/Common.h"

template<class T>
class SortedList : public List<T>
{
public:
    void Add(T t)
    {
		if (!Add(t, true))
			ASSERT_FAIL(); // TODO: application-specific,
						   // that's how we use SortedList<>
    }
		
private:
    bool Add(T t, bool unique)
	{
        ListNode<T>*  node;
        ListNode<T>** curr = &this->head;

		while(true)
		{
        	if (*curr == NULL || LessThan(t, (*curr)->data)) {
	            node = new ListNode<T>;
				node->data = t;
				node->next = *curr;
				if (curr != &this->head)
				{
					node->prev =
								(ListNode<T>*)
								((char*)curr - offsetof(ListNode<T>, next));
				}
				else
				{
					node->prev = NULL;
				}
				if (*curr != NULL)
					(*curr)->prev = node;
				if (*curr == NULL)
					this->tail = node;
				*curr = node;
				this->length++;		
				return true;
	        } 
			else
			{
	            if (unique && (*curr)->data == t)
					return false;
	            curr = &(*curr)->next;
	        }
		}
		
		ASSERT_FAIL();
    }
};

#endif
