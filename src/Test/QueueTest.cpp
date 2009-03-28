#include "Test.h"
#include "System/Containers/Queue.h"
#include "System/Containers/LinkedList.h"
#include "System/Buffer.h"

typedef DynArray<32> Buffer;
int BasicQueueTest()
{
	Buffer *buffer;
	Queue<Buffer, &Buffer::next> q;

	for (int i = 0; i < 3; i++)
	{
		buffer = new Buffer();
		buffer->Printf("%i", i);
		q.Add(buffer);
	}
	
	for (int  i = 0; i < 3; i++)
	{
		buffer = q.Get();
		TEST_LOG("%.*s", buffer->length, buffer->buffer);
		delete buffer;
	}

	return TEST_SUCCESS;
}

class Elem
{
public:
	// this node will be part of listA
	Elem*			nextA;
	Elem*			prevA;
	Buffer			buffer;
	// this node will be part of listB
	Elem*			nextB;
	Elem*			prevB;
	
	int				i;
	LinkedListNode<Elem>	node;
	LLNode<Elem>	llnode;
};

int BasicLinkedListTest()
{
	LinkedList2<Elem, &Elem::nextA, &Elem::prevA>	listA;
	LinkedList2<Elem, &Elem::nextB, &Elem::prevB>	listB;
	LinkedList<Elem, &Elem::node> listC;
	LList<Elem, &Elem::llnode> llist;
	
	for (int i = 0; i < 3; i++)
	{
		Elem* elem;
		
		elem = new Elem;
		elem->buffer.Printf("%i", i);
		//listC.Append(*elem);
		llist.Add(*elem);
	}
	
	Elem* next = NULL;
	for (Elem* e = listC.Head(); e; e = next)
	{
		TEST_LOG("%.*s", e->buffer.length, e->buffer.data);
		next = listC.Remove(e);
		delete e;
	}
	
	return TEST_SUCCESS;
}

TEST_MAIN(BasicQueueTest, BasicLinkedListTest);
