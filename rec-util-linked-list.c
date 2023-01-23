/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 *  - Description: Implementation of a list.
 *
 *      To create a list, use either util_linked_list_create() function for new list,
 *      or util_linked_list_copy() to copy a list. In both ways, the returned object
 *      has to be released with util_linked_list_release() function.
 *
 *      To add elements to the list, use the util_linked_list_push_element() function.
 *
 *      To iterate over the list:
 *          LinkedListNode *it = util_linked_list_get_first_element();
 *          while( NULL != it )
 *          {
 *              // use the element
 *              it = util_linked_list_iterate_to_next_element( it );
 *          }
 *
 *      To access data in the element, use functions util_linked_list_get_data() and
 *      util_linked_list_get_data_size()
 *
 */

#include "rec-util-linked-list.h"

#include <stdlib.h>
#include <string.h>

/* Node type in the list.                                               */
/* This structure uses struct hack to minimize number of allocations.   */
struct LinkedListNode {
	struct LinkedListNode *nextNode;

	unsigned int dataSize;
	char data[0];
};

struct LinkedList {
	/* number of elements in the list */
	unsigned int n;

	/* points to the first element */
	struct LinkedListNode *begin;

	/* points to the last element */
	struct LinkedListNode *end;
};

/**
 * Create an empty linked list. When done with the linked list, it has to be released with
 * util_linked_list_release() function.
 *
 * @return new linked list
 *
 * */
struct LinkedList *util_linked_list_create()
{
	struct LinkedList *list = malloc(sizeof(struct LinkedList));

	list->n = 0;
	list->begin = NULL;
	list->end = NULL;

	return list;
}

/**
 * Copy linked list. It copies each node, therefore the copied linked list has to be released
 * with util_linked_list_release() function.
 *
 * @param[in] list linked list to copy
 *
 * @return copied linked list
 *
 * */
const struct LinkedList *util_linked_list_copy(const struct LinkedList *list)
{
	struct LinkedList *copy = util_linked_list_create();

	const struct LinkedListNode *it =
		util_linked_list_get_first_element(list);

	while (NULL != it) {
		util_linked_list_push_element(copy, it->data, it->dataSize);

		it = util_linked_list_iterate_to_next_element(list, it);
	}

	copy->n = list->n;

	return copy;
}

/**
 * Release the linked list.
 *
 * @param[in] list linked list to release
 * */
void util_linked_list_release(const struct LinkedList *list)
{
	const struct LinkedListNode *it =
		util_linked_list_get_first_element(list);

	while (NULL != it) {
		const struct LinkedListNode *next =
			util_linked_list_iterate_to_next_element(list, it);

		free((void *)it);

		it = next;
	}

	free((void *)list);
}

/**
 * Pushes new element at the end of the list.
 *
 * @param[in] list linked list where to push the element
 * @param[in] data pointer to data to store into the list
 * @param[in] dataSize size of data in bytes
 * */
void util_linked_list_push_element(struct LinkedList *list, const void *data,
				   const unsigned int dataSize)
{
	struct LinkedListNode *node =
		malloc(sizeof(struct LinkedListNode) + dataSize);

	memcpy(node->data, data, dataSize);
	node->dataSize = dataSize;
	node->nextNode = NULL;

	if (NULL == list->begin) {
		list->begin = node;
		list->end = node;
	} else {
		list->end->nextNode = node;
		list->end = node;
	}

	++list->n;
}

/**
 * Return number of elements in the linked list.
 *
 * @return number of elements in the linked list
 */
unsigned int util_linked_list_get_size(const struct LinkedList *list)
{
	if (NULL == list) {
		return 0;
	}

	return list->n;
}

/**
 * Returns the first element in the linked list.
 *
 * @param[in] list linked list to get the first element
 *
 * @return pointer to the first element in the linked list, or NULL if empty
 * */
const struct LinkedListNode *
util_linked_list_get_first_element(const struct LinkedList *list)
{
	if (NULL == list) {
		return NULL;
	}

	return list->begin;
}

/**
 * Iterate to next element in the linked list.
 *
 * @param[in] list list over which is being iterated
 * @param[in] node current element
 *
 * @return pointer to the next node in the linked list that comes after the current node,
 *         or NULL if no more elements
 * */
const struct LinkedListNode *
util_linked_list_iterate_to_next_element(const struct LinkedList *list,
					 const struct LinkedListNode *node)
{
	if ((NULL == list) || (NULL == node) || (list->end == node)) {
		return NULL;
	}

	return node->nextNode;
}

/**
 * Get pointer to the data in the element.
 *
 * @param[in] node element in the linked list
 *
 * @return pointer to data in the element
 * */
const void *util_linked_list_get_data(const struct LinkedListNode *node)
{
	if (NULL == node) {
		return NULL;
	}

	return node->data;
}

/**
 * Get data size in the element.
 *
 * @param[in] node element in the linked list
 *
 * @return data size of the data in the element
 * */
unsigned int util_linked_list_get_data_size(const struct LinkedListNode *node)
{
	if (NULL == node) {
		return 0;
	}

	return node->dataSize;
}
