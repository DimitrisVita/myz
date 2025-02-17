#include "ADTList.h"
#include <stdlib.h>

// Creates a new list and returns it.
List list_create(DestroyFunc destroy_value) {
	List list = malloc(sizeof(*list));
	list->dummy = malloc(sizeof(*(list->dummy)));
	list->dummy->next = NULL;
	list->last = list->dummy;
	list->size = 0;
	list->destroy_value = destroy_value;
	return list;
}

// Sets the destroy function for the list.
void list_set_destroy_value(List list, DestroyFunc destroy_value) {
	list->destroy_value = destroy_value;
}

// Destroys the list and frees all allocated memory.
void list_destroy(List list) {
	ListNode node = list->dummy;
	while (node != NULL) {
		ListNode next = node->next;
		if (list->destroy_value != NULL && node != list->dummy) {
			list->destroy_value(node->value);
		}
		free(node);
		node = next;
	}
	free(list);
}

// Inserts a new node with the given value after the specified node.
void list_insert_after(List list, ListNode node, void* value) {
	ListNode new_node = malloc(sizeof(*new_node));
	new_node->value = value;
	new_node->next = node->next;
	node->next = new_node;
	if (node == list->last) {
		list->last = new_node;
	}
	list->size++;
}

// Removes the node after the specified node.
void list_remove_after(List list, ListNode node) {
	if (node == NULL) {
		node = list->dummy;
	}
	ListNode to_remove = node->next;
	if (to_remove != NULL) {
		node->next = to_remove->next;
		if (list->destroy_value != NULL) {
			list->destroy_value(to_remove->value);
		}
		if (to_remove == list->last) {
			list->last = node;
		}
		free(to_remove);
		list->size--;
	}
}

// Finds and returns the value in the list that matches the given value using the compare function.
void* list_find_value(List list, void* value, int (*compare)(void*, void*)) {
	for (ListNode node = list->dummy->next; node != NULL; node = node->next) {
		if (compare(node->value, value) == 0) {
			return node->value;
		}
	}
	return NULL;
}

// Returns the size of the list.
int list_size(List list) {
	return list->size;
}

// Returns the first node in the list.
ListNode list_first(List list) {
	return list->dummy->next;
}

// Returns the next node in the list.
ListNode list_next(ListNode node) {
	return node->next;
}

// Returns the last node in the list.
ListNode list_last(List list) {
	return list->last;
}

// Returns the value of the given node.
void* list_value(ListNode node) {
	return node->value;
}

// Finds and returns the node in the list that matches the given value using the compare function.
ListNode list_find(List list, void* value, int (*compare)(void*, void*)) {
	for (ListNode node = list->dummy->next; node != NULL; node = node->next) {
		if (compare(node->value, value) == 0) {
			return node;
		}
	}
	return NULL;
}