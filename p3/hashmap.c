#include "hashmap.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mapreduce.h"

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

pthread_rwlock_t lock;
// for some reason vs code doesn't think this is a thing but the compiler says it is sooo

HashMap* MapInit(void) {
  HashMap* hashmap = (HashMap*)malloc(sizeof(HashMap));
  hashmap->contents = (MapPair**)calloc(MAP_INIT_CAPACITY, sizeof(MapPair*));
  hashmap->capacity = MAP_INIT_CAPACITY;
  hashmap->size = 0;
  pthread_rwlock_init(&lock, NULL);
  return hashmap;
}

void MapPut(HashMap* hashmap, char* key, void* value, int value_size) {
  pthread_rwlock_rdlock(&lock);
  if (hashmap->size > (hashmap->capacity / 2)) {
    pthread_rwlock_unlock(&lock);
    if (resize_map(hashmap) < 0) {
      exit(0);
    }
  } else {
    pthread_rwlock_unlock(&lock);
  }

  MapPair* newpair = (MapPair*)malloc(sizeof(MapPair));
  int h;

  newpair->key = strdup(key);
  newpair->value = (void*)malloc(value_size);
  memcpy(newpair->value, value, value_size);
  
  pthread_rwlock_wrlock(&lock);
  h = Hash(key, hashmap->capacity);

  while (hashmap->contents[h] != NULL) {
    // if keys are equal, update
    if (!strcmp(key, hashmap->contents[h]->key)) {
      free(hashmap->contents[h]);
      hashmap->contents[h] = newpair;
      pthread_rwlock_unlock(&lock);
      return;
    }
    h++;
    if (h == hashmap->capacity) h = 0;
  }

  // key not found in hashmap, h is an empty slot
  hashmap->contents[h] = newpair;
  hashmap->size += 1;
  pthread_rwlock_unlock(&lock);
}

char* MapGet(HashMap* hashmap, char* key) {
  int h = Hash(key, hashmap->capacity);
  pthread_rwlock_rdlock(&lock);
  while (hashmap->contents[h] != NULL) {  // iterate over the hashmap
    if (!strcmp(key, hashmap->contents[h]->key)) {
      pthread_rwlock_unlock(&lock);
      return hashmap->contents[h]->value;
    }
    h++;
    if (h == hashmap->capacity) {
      h = 0;
    }
  }
  pthread_rwlock_unlock(&lock);
  return NULL;
}

size_t MapSize(HashMap* map) { return map->size; }

int resize_map(HashMap* map) {
  MapPair** temp;

  pthread_rwlock_wrlock(&lock);
  size_t newcapacity = map->capacity * 2;  // double the capacity
  pthread_rwlock_unlock(&lock);

  // allocate a new hashmap table
  temp = (MapPair**)calloc(newcapacity, sizeof(MapPair*));
  if (temp == NULL) {
    printf("Malloc error! %s\n", strerror(errno));
    return -1;
  }

  size_t i;
  int h;
  MapPair* entry;
  pthread_rwlock_wrlock(&lock);
  // rehash all the old entries to fit the new table
  for (i = 0; i < map->capacity; i++) {
    if (map->contents[i] != NULL)
      entry = map->contents[i];
    else
      continue;
    h = Hash(entry->key, newcapacity);
    while (temp[h] != NULL) {
      h++;
      if (h == newcapacity) h = 0;
    }
    temp[h] = entry;
  }

  // free the old table
  free(map->contents);
  // update contents with the new table, increase hashmap capacity
  map->contents = temp;
  map->capacity = newcapacity;
  pthread_rwlock_unlock(&lock);
  return 0;
}

// FNV-1a hashing algorithm
// https://en.wikipedia.org/wiki/Fowler-Noll-Vo_hash_function#FNV-1a_hash
size_t Hash(char* key, size_t capacity) {
  size_t hash = FNV_OFFSET;
  for (const char* p = key; *p; p++) {
    hash ^= (size_t)(unsigned char)(*p);
    hash *= FNV_PRIME;
    hash ^= (size_t)(*p);
  }
  return (hash % capacity);
}
