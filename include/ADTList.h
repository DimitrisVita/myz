#pragma once

#include "common.h"

typedef void (*DestroyFunc)(void* value);

typedef struct list* List;
typedef struct list_node* ListNode;

struct list {
	ListNode dummy;
	ListNode last;
	int size;
	DestroyFunc destroy_value;
};

struct list_node {
	ListNode next;
	void* value;
};

// Create a new list
List list_create(DestroyFunc destroy_value);

// Set the destroy value function
void list_set_destroy_value(List list, DestroyFunc destroy_value);

// Destroy the list
void list_destroy(List list);

// Insert a new node after the given node
void list_insert_after(List list, ListNode node, void* value);

// Insert a new node before the given node
void list_remove_after(List list, ListNode node);

// Find the value in the list
void* list_find_value(List list, void* value, int (*compare)(void*, void*));

//  Get the size of the list
int list_size(List list);

// Get the first node of the list
ListNode list_first(List list);

// Get the next node of the given node
ListNode list_next(List list, ListNode node);

// Get the last node of the list
ListNode list_last(List list);

// Get the value of the given node
void* list_value(List list, ListNode node);

// Find the node with the given value
ListNode list_find(List list, void* value, int (*compare)(void*, void*));

// Destroy archive entry
void destroy_archive_entry(void *value);
