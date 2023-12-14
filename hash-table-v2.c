#include "hash-table-base.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/queue.h>

#include <pthread.h>

struct list_entry {
	const char *key;
	uint32_t value;
	SLIST_ENTRY(list_entry) pointers;
};

SLIST_HEAD(list_head, list_entry);

struct hash_table_entry {
	struct list_head list_head;
	pthread_mutex_t mutex;
};

struct hash_table_v2 {
	struct hash_table_entry entries[HASH_TABLE_CAPACITY];
};

struct hash_table_v2 *hash_table_v2_create()
{
	struct hash_table_v2 *hash_table = calloc(1, sizeof(struct hash_table_v2));
	assert(hash_table != NULL);
	pthread_mutexattr_t mutex_attr;
	int rc = pthread_mutexattr_init(&mutex_attr);
	if (rc != 0)
	{
		errno = rc;
		exit(1);
	}
	rc = pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK);
	if (rc != 0)
	{
		errno = rc;
		exit(1);
	}
	for (size_t i = 0; i < HASH_TABLE_CAPACITY; ++i) {
		struct hash_table_entry *entry = &hash_table->entries[i];
		SLIST_INIT(&entry->list_head);
		rc = pthread_mutex_init(&entry->mutex, &mutex_attr);
		if (rc != 0)
		{
			errno = rc;
			exit(1);
		}
	}
	rc = pthread_mutexattr_destroy(&mutex_attr);
	if (rc != 0)
	{
		errno = rc;
		exit(1);
	}
	return hash_table;
}

static struct hash_table_entry *get_hash_table_entry(struct hash_table_v2 *hash_table,
                                                     const char *key)
{
	assert(key != NULL);
	uint32_t index = bernstein_hash(key) % HASH_TABLE_CAPACITY;
	struct hash_table_entry *entry = &hash_table->entries[index];
	return entry;
}

static struct list_entry *get_list_entry(struct hash_table_v2 *hash_table,
                                         const char *key,
                                         struct list_head *list_head)
{
	assert(key != NULL);

	struct list_entry *entry = NULL;
	
	SLIST_FOREACH(entry, list_head, pointers) {
	  if (strcmp(entry->key, key) == 0) {
	    return entry;
	  }
	}
	return NULL;
}

bool hash_table_v2_contains(struct hash_table_v2 *hash_table,
                            const char *key)
{
	struct hash_table_entry *hash_table_entry = get_hash_table_entry(hash_table, key);
	struct list_head *list_head = &hash_table_entry->list_head;
	struct list_entry *list_entry = get_list_entry(hash_table, key, list_head);
	return list_entry != NULL;
}

void hash_table_v2_add_entry(struct hash_table_v2 *hash_table,
                             const char *key,
                             uint32_t value)
{
	struct hash_table_entry *hash_table_entry = get_hash_table_entry(hash_table, key);
	int rc;
	for (;;)
	{
		rc = pthread_mutex_lock(&hash_table_entry->mutex);
		if(rc == 0)
			break;
		if(rc == EINVAL)
		{
			errno = rc;
			exit(1);
		}
	}
	struct list_head *list_head = &hash_table_entry->list_head;
	struct list_entry *list_entry = get_list_entry(hash_table, key, list_head);

	/* Update the value if it already exists */
	if (list_entry != NULL) {
		list_entry->value = value;
		rc = pthread_mutex_unlock(&hash_table_entry->mutex);
		if (rc != 0)
		{
			errno = rc;
			exit(1);
		}
		return;
	}
	rc = pthread_mutex_unlock(&hash_table_entry->mutex);
	if (rc != 0)
	{
		errno = rc;
		exit(1);
	}
	
	list_entry = calloc(1, sizeof(struct list_entry));
	list_entry->key = key;
	list_entry->value = value;
	for (;;)
	{
		int rc = pthread_mutex_lock(&hash_table_entry->mutex);
		if(rc == 0)
			break;
		if(rc == EINVAL)
		{
			errno = rc;
			exit(1);
		}
	}
	SLIST_INSERT_HEAD(list_head, list_entry, pointers);
	rc = pthread_mutex_unlock(&hash_table_entry->mutex);
	if (rc != 0)
	{
		errno = rc;
		exit(1);
	}
}

uint32_t hash_table_v2_get_value(struct hash_table_v2 *hash_table,
                                 const char *key)
{
	struct hash_table_entry *hash_table_entry = get_hash_table_entry(hash_table, key);
	struct list_head *list_head = &hash_table_entry->list_head;
	struct list_entry *list_entry = get_list_entry(hash_table, key, list_head);
	assert(list_entry != NULL);
	return list_entry->value;
}

void hash_table_v2_destroy(struct hash_table_v2 *hash_table)
{
	for (size_t i = 0; i < HASH_TABLE_CAPACITY; ++i) {
		struct hash_table_entry *entry = &hash_table->entries[i];
		int rc = pthread_mutex_destroy(&entry->mutex);
		if (rc != 0)
		{
			errno = rc;
			exit(1);
		}
		struct list_head *list_head = &entry->list_head;
		struct list_entry *list_entry = NULL;
		while (!SLIST_EMPTY(list_head)) {
			list_entry = SLIST_FIRST(list_head);
			SLIST_REMOVE_HEAD(list_head, pointers);
			free(list_entry);
		}
	}
	free(hash_table);
}
