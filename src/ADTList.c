#include "ADTList.h"
#include <stdlib.h>

// ...existing code...

List list_create(DestroyFunc destroy_value) {
	List list = malloc(sizeof(*list));
	list->dummy = malloc(sizeof(*(list->dummy)));
	list->dummy->next = NULL;
	list->last = list->dummy;
	list->size = 0;
	list->destroy_value = destroy_value;
	return list;
}

void list_set_destroy_value(List list, DestroyFunc destroy_value) {
	list->destroy_value = destroy_value;
}

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

void list_remove_after(List list, ListNode node) {
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

void* list_find_value(List list, void* value, int (*compare)(void*, void*)) {
	for (ListNode node = list->dummy->next; node != NULL; node = node->next) {
		if (compare(node->value, value) == 0) {
			return node->value;
		}
	}
	return NULL;
}

int list_size(List list) {
	return list->size;
}

ListNode list_first(List list) {
	return list->dummy->next;
}

ListNode list_next(List list, ListNode node) {
	return node->next;
}

ListNode list_last(List list) {
	return list->last;
}

void* list_value(List list, ListNode node) {
	return node->value;
}

ListNode list_find(List list, void* value, int (*compare)(void*, void*)) {
	for (ListNode node = list->dummy->next; node != NULL; node = node->next) {
		if (compare(node->value, value) == 0) {
			return node;
		}
	}
	return NULL;
}

