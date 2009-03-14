#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "../BlockAlloc.h"

/*
===============================================================================

Hash table template.
A hash function must be provided for the Key type with the following signature:
size_t Hash(const Key& key).

===============================================================================
*/

#define HT_MIN_LOAD		0.1f
#define HT_MAX_LOAD		0.75f


template<class Key, class Value>
class HashTable;


template<class K, class V>
class HashNode
{
friend class HashTable<K, V>;

public:
	const K &		Key() const { return key; }
	V &				Value() { return value; }

private:
	HashNode<K, V>	*next;
	K				key;
	V				value;
};


template<class Key, class Value>
class HashTable
{
public:
	typedef HashNode<Key, Value>	Node;

							HashTable(int initialSize = 16);
							HashTable(const HashTable<Key, Value> &src);
							~HashTable();

	HashTable<Key, Value> &	operator=(const HashTable<Key, Value> &other);

	int						Num() const;		// returns number of elements in hash table

	size_t					Allocated() const;	// returns total size of allocated memory
	size_t					Size() const;		// returns total size of allocated memory including size of hash table type 

	void					Clear();
	bool					IsEmpty() const;

	Value *					Get(const Key &key);						// returns NULL if not found
	const Value *			Get(const Key &key) const;					// returns NULL if not found
	bool					Get(const Key &key, Value &value) const;	// return false if not found
	Value *					Set(const Key &key, const Value &value);
	void					Remove(const Key &key);

	Node *					First() const;	// returns first node for iterating through the contents
	Node *					Next(const Node *node) const;
	void					DeleteContents();

private:
	Node					**heads;
	int						num;
	int						tableSize;
	int						tableMask;

	BlockAlloc<Node, 16>	allocator;	// TODO: tune this value

	size_t					GetHash(const Key &key) const;

public:
	void					Resize(int numElements);
};


int CeilPowerOfTwo(int x);		// round x up to the nearest power of 2 


/*
====================
HashTable<Key, Value>::HashTable
====================
*/
template<class Key, class Value>
HashTable<Key, Value>::HashTable(int initialSize)
{
	num = 0;

	tableSize = initialSize;
	tableSize = (int)(initialSize / HT_MAX_LOAD);
	tableSize = CeilPowerOfTwo(tableSize);
	tableMask = tableSize - 1;

	heads = new Node *[tableSize];
	memset(heads, 0, tableSize * sizeof(Node *));
}


/*
====================
HashTable<Key, Value>::HashTable
====================
*/
template<class Key, class Value>
HashTable<Key, Value>::HashTable(const HashTable<Key, Value> &src)
{
	num = 0;
	tableSize = 0;
	tableMask = 0;
	heads = NULL;

	*this = src;
}

  
/*
====================
HashTable<Key, Value>::~HashTable
====================
*/
template<class Key, class Value>
HashTable<Key, Value>::~HashTable()
{
	Clear();
	delete[] heads;
}

  
/*
====================
HashTable<Key, Value>::operator=
====================
*/
template<class Key, class Value>
HashTable<Key, Value> &	HashTable<Key, Value>::operator=(const HashTable<Key, Value> &other)
{
	int		i;
	Node	*node, *newNode;
	Node	**prev;

	Clear();
	delete[] heads;

	num = other.num;
	tableSize = other.tableSize;
	tableMask = other.tableMask;
	heads = new Node *[tableSize];

	for(i = 0; i < tableSize; i++)
	{
		if (!other.heads[i])
		{
			heads[i] = NULL;
			continue;
		}

		prev = &heads[i];
		for (node = other.heads[i]; node; node = node->next)
		{
			newNode = allocator.Alloc();
			newNode->key = node->key;
			newNode->value = node->value;
			newNode->next = NULL;
			*prev = newNode;
			prev = &newNode->next;
		}
	}

	return *this;
}


/*
====================
HashTable<Key, Value>::Num
====================
*/
template<class Key, class Value>
int HashTable<Key, Value>::Num() const
{
	return num;
}


/*
====================
HashTable<Key, Value>::Allocated
====================
*/
template<class Key, class Value>
size_t HashTable<Key, Value>::Allocated() const
{
	return tableSize * sizeof(Node *) + allocator.GetTotalCount() * sizeof(Node);
}


/*
====================
HashTable<Key, Value>::Size
====================
*/
template<class Key, class Value>
size_t HashTable<Key, Value>::Size() const
{
	return sizeof(*this) + Allocated();
}


/*
====================
HashTable<Key, Value>::Clear
====================
*/
template<class Key, class Value>
void HashTable<Key, Value>::Clear()
{
	int		i;
	Node	*node, *next;

	for (i = 0; i < tableSize; i++)
	{
		for (node = heads[i]; node; node = next)
		{
			next = node->next;
			allocator.Free(node);
		}
		heads[i] = NULL;
	}
	num = 0;
}


/*
====================
HashTable<Key, Value>::IsEmpty
====================
*/
template<class Key, class Value>
bool HashTable<Key, Value>::IsEmpty() const
{
	return (num == 0);
}


/*
====================
HashTable<Key, Value>::Get
====================
*/
template<class Key, class Value>
Value * HashTable<Key, Value>::Get(const Key &key)
{
	size_t	hash;
	Node	*node;

	hash = GetHash(key);
	for (node = heads[hash]; node; node = node->next)
	{
		if (node->key == key)
			return &node->value;
	}
	return NULL;
}


/*
====================
HashTable<Key, Value>::Get
====================
*/
template<class Key, class Value>
const Value * HashTable<Key, Value>::Get(const Key &key) const
{
	size_t	hash;
	Node	*node;

	hash = GetHash(key);
	for (node = heads[hash]; node; node = node->next)
	{
		if (node->key == key)
			return &node->value;
	}
	return NULL;
}


/*
====================
HashTable<Key, Value>::Get
====================
*/
template<class Key, class Value>
bool HashTable<Key, Value>::Get(const Key &key, Value &value) const
{
	const Value		*val;

	val = Get(key);
	if (!val)
		return false;
	
	value = *val;
	return true;
}


/*
====================
HashTable<Key, Value>::Set
====================
*/
template<class Key, class Value>
Value * HashTable<Key, Value>::Set(const Key &key, const Value &value)
{
	size_t	hash;
	Node	*node;

	hash = GetHash(key);
	for (node = heads[hash]; node; node = node->next)
	{
		if (node->key == key)
		{
			node->value = value;
			return &node->value;
		}
	}

	node = allocator.Alloc();
	node->key = key;
	node->value = value;

	node->next = heads[hash];
	heads[hash] = node;
	num++;

	// grow table if it's too small
	if (num > tableSize * HT_MAX_LOAD)
		Resize(num);

	return &node->value;
}


/*
====================
HashTable<Key, Value>::Remove
====================
*/
template<class Key, class Value>
void HashTable<Key, Value>::Remove(const Key &key)
{
	size_t	hash;
	Node	*node, *next;
	Node	**prev;

	hash = GetHash(key);

	prev = &heads[hash];
	for (node = heads[hash]; node; node = next)
	{
		next = node->next;
		if (node->key == key)
		{
			*prev = next;
			allocator.Free(node);
			num--;
			break;
		}
		prev = &node->next;
	}

	// shrink table if it's too big
	if (num < tableSize * HT_MIN_LOAD)
		Resize(num);
}


/*
====================
HashTable<Key, Value>::First
====================
*/
template<class Key, class Value>
HashNode<Key, Value> * HashTable<Key, Value>::First() const
{
	int		i;

	for (i = 0; i < tableSize; i++)
	{
		if (heads[i])
			return heads[i];
	}
	return NULL;
}


/*
====================
HashTable<Key, Value>::Next
====================
*/
template<class Key, class Value>
HashNode<Key, Value> * HashTable<Key, Value>::Next(const Node *node) const
{
	int		i;
	size_t	hash;

	if (node->next)
		return node->next;

	hash = GetHash(node->key);
	for (i = hash + 1; i < tableSize; i++)
	{
		if (heads[i])
			return heads[i];
	}
	return NULL;
}


/*
====================
HashTable<Key, Value>::DeleteContents
====================
*/
template<class Key, class Value>
void HashTable<Key, Value>::DeleteContents()
{
	int		i;
	Node	*node;

	for (i = 0; i < tableSize; i++)
	{
		for (node = heads[i]; node; node = node->next)
			delete node->value;
	}
	Clear();
}


/*
====================
HashTable<Key, Value>::GetHash
====================
*/
template<class Key, class Value>
size_t HashTable<Key, Value>::GetHash(const Key &key) const
{
	return Hash(key) & tableMask;
}


/*
====================
HashTable<Key, Value>::Resize
====================
*/
template<class Key, class Value>
void HashTable<Key, Value>::Resize(int numElements)
{
	Node	**tmp;
	Node	*node, *next;
	int		i, newSize, newMask;
	size_t	hash;

	assert(numElements >= num);
	if (numElements < 10)
		numElements = 10;

	newSize = (int)(numElements / HT_MAX_LOAD);
	newSize = CeilPowerOfTwo(newSize);
	newMask = newSize - 1;
	if (newSize == tableSize)
		return;

	tmp = new Node *[newSize];
	memset(tmp, 0, newSize * sizeof(Node *));
	for (i = 0; i < tableSize; i++)
	{
		for (node = heads[i]; node; node = next)
		{
			next = node->next;
			hash = 	Hash(node->key) & newMask;
			node->next = tmp[hash];
			tmp[hash] = node;
		}
	}
	delete[] heads;
	heads = tmp;
	tableSize = newSize;
	tableMask = newMask;
}


/*
====================
CeilPowerOfTwo
====================
*/
inline int CeilPowerOfTwo(int x)
{
	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	x++;
	return x;
}

#endif
