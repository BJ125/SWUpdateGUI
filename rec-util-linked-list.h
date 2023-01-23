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

#ifndef REC_UTIL_LINKED_LIST_H
#define REC_UTIL_LINKED_LIST_H

#ifdef __cplusplus
extern "C" {
#endif

struct LinkedListNode;
struct LinkedList;

struct LinkedList *util_linked_list_create();
const struct LinkedList *util_linked_list_copy(const struct LinkedList *list);

void util_linked_list_release(const struct LinkedList *list);

void util_linked_list_push_element(struct LinkedList *list, const void *data,
				   const unsigned int dataSize);

unsigned int util_linked_list_get_size(const struct LinkedList *list);

const struct LinkedListNode *
util_linked_list_get_first_element(const struct LinkedList *list);
const struct LinkedListNode *
util_linked_list_iterate_to_next_element(const struct LinkedList *list,
					 const struct LinkedListNode *node);

const void *util_linked_list_get_data(const struct LinkedListNode *node);
unsigned int util_linked_list_get_data_size(const struct LinkedListNode *node);

#ifdef __cplusplus
}
#endif

#endif /* REC_UTIL_LINKED_LIST_H */
