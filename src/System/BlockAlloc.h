#ifndef BLOCKALLOC_H
#define BLOCKALLOC_H

/*
===============================================================================

Block based allocator for fixed size objects.

All objects of the 'type' are properly constructed.
However, the constructor is not called for re-used objects.

===============================================================================
*/

template<class Type, int blockSize>
class BlockAlloc
{
public:
						BlockAlloc();
						~BlockAlloc();

	void				Shutdown();

	Type *				Alloc();
	void				Free(Type *element);

	int					GetTotalCount() const { return total; }
	int					GetAllocCount() const { return active; }
	int					GetFreeCount() const { return total - active; }

private:

	struct block_t;

	struct element_t : public Type
	{
		union
		{
			element_t	*next;		// next element in free list
			block_t		*block;		// owner block if allocated
		} ptr;
	};

	struct block_t
	{
		block_t			*prev;
		block_t			*next;
		int				numFree;
		element_t		*free;
		element_t		elements[blockSize];
	};

	block_t				*blocks;	// free blocks are always at the front 
	int					total;
	int					active;

									BlockAlloc(const BlockAlloc<Type, blockSize> &src);
	BlockAlloc<Type, blockSize> &	operator=(const BlockAlloc<Type, blockSize> &other);
};


/*
====================
BlockAlloc<Type, blockSize>::BlockAlloc
====================
*/
template<class Type, int blockSize>
BlockAlloc<Type, blockSize>::BlockAlloc()
{
	blocks = NULL;
	total = active = 0;
}


/*
====================
BlockAlloc<Type, blockSize>::~BlockAlloc
====================
*/
template<class Type, int blockSize>
BlockAlloc<Type, blockSize>::~BlockAlloc()
{
	Shutdown();
}


/*
====================
BlockAlloc<Type, blockSize>::Alloc
====================
*/
template<class Type, int blockSize>
Type * BlockAlloc<Type, blockSize>::Alloc()
{
	int			i;
	block_t		*block;
	element_t	*element, *next;

	block = blocks;
	if (!block || !block->free)
	{
		block = new block_t;
		if (blocks)
		{
			block->prev = blocks->prev;
			block->next = blocks;
		}
		else
		{
			block->prev = block;
			block->next = block;
		}
		block->prev->next = block;
		block->next->prev = block;
		blocks = block;

		next = NULL;
		for (i = blockSize - 1; i >= 0; i--)
		{
			block->elements[i].ptr.next = next;
			next = &block->elements[i];
		}
		block->free = block->elements;
		block->numFree = blockSize;

		total += blockSize;
	}
	active++;

	element = block->free;
	block->free = element->ptr.next;
	element->ptr.block = block;
	block->numFree--;

	// if the block is full, move it to the end of the list
	if (!block->free)
		blocks = block->next;

	return element;
}


/*
====================
BlockAlloc<Type, blockSize>::Free
====================
*/
template<class Type, int blockSize>
void BlockAlloc<Type, blockSize>::Free(Type *t)
{
	element_t	*element;
	block_t		*block;

	element = static_cast<element_t *>(t);
	block = element->ptr.block;

	// link the element to the head of the free list
	element->ptr.next = block->free;
	block->free = element;
	block->numFree++;

	// free the block if it's all empty
	if (block->numFree == blockSize)
	{
		if (blocks == block)
			blocks = block->next != block ? block->next : NULL;

		block->prev->next = block->next;
		block->next->prev = block->prev;
		delete block;

		total -= blockSize;
	}
	active--;
}


/*
====================
BlockAlloc<Type, blockSize>::Shutdown
====================
*/
template<class Type, int blockSize>
void BlockAlloc<Type, blockSize>::Shutdown()
{
	block_t		*block, *first, *next;

	if (blocks)
	{
		first = block = blocks;
		do
		{
			next = block->next;
			delete block;
			block = next;
		} while (block != first);
		blocks = NULL;
	}
	total = active = 0;
}

#endif
