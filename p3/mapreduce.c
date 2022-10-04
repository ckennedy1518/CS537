#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashmap.h"
#include "mapreduce.h"

int filesRemaining;  // global counter for how many more files must be mapped
pthread_mutex_t filesRemaining_lock;

typedef struct pair {  // linked list in each bucket
  char* key;
  char* value;
  struct pair* next;
} pair;

typedef struct list {
  pair* head;
} list;

list* internal;        // internal data structure
size_t internal_size;  // aka num_partitions aka num_reducers
pthread_mutex_t* bucket_locks;
Partitioner party;

typedef struct objToPass {  // struct is used to reduce the arguments in thread creation
  Mapper map;
  char** argv;
} objToPass;

typedef struct reducePasser {
  Reducer reduce;
  Getter get;
  int partition_number;
} reducePasser;

int* bucketBeenAdded;

void init_list(size_t num_partitions) {
  internal = (list*) malloc(num_partitions * sizeof(list));
  if (internal == NULL) {
    printf("Error with malloc\n");
    exit(1);
  }
  for (int i = 0; i < num_partitions; i++) {
    internal[i].head = (pair*) calloc(1, sizeof(pair));
    internal[i].head->key = NULL;
  }
  internal_size = num_partitions;
  pthread_mutex_init(&filesRemaining_lock, NULL);
  bucket_locks =
      (pthread_mutex_t*)malloc(num_partitions * sizeof(pthread_mutex_t));
  if (bucket_locks == NULL) {
    printf("Error with malloc\n");
    exit(1);
  }
  for (int i = 0; i < num_partitions; i++) {
    pthread_mutex_init(&(bucket_locks[i]), NULL);
  }
  bucketBeenAdded = (int*) calloc(num_partitions, sizeof(int));
  if (bucketBeenAdded == NULL) {
    printf("Error with malloc\n");
    exit(1);
  }
}

/**
 * @brief addeds a pair to the internal data function
 * @param key - read the man pages
 * @param value - read the man pages
 */
void MR_Emit(char* key, char* value) {
  pair* kv = (pair*)malloc(sizeof(pair));
  if (kv == NULL) {
    printf("Error with malloc\n");
    exit(1);
  }
  kv->key = malloc(strlen(key) * sizeof(char));
  kv->value = malloc(strlen(value) * sizeof(char));
  strcpy(kv->key, key);
  strcpy(kv->value, value);
  if (!kv->key || !kv->value)
    return;
  int insertAt = party(key, internal_size);

  pthread_mutex_lock(&(bucket_locks[insertAt]));
  pair* temp = internal[insertAt].head;
  internal[insertAt].head = kv;
  if (bucketBeenAdded[insertAt]) {
    kv->next = temp;
  } else {
    kv->next = NULL;
    (bucketBeenAdded[insertAt])++;
  }
  pthread_mutex_unlock(&(bucket_locks[insertAt]));
}

unsigned long MR_DefaultHashPartition(char* key, int num_partitions) {
  unsigned long hash = 5381;
  int c;
  while ((c = *key++) != '\0') hash = hash * 33 + c;
  return hash % num_partitions;
}

/**
 * @brief This function assigns a file to mappers
 *
 * @param otp - contains array of file names and mapper function pointer
 */
void mapper(objToPass* otp) {
  pthread_t id;
  while (1) {
    pthread_mutex_lock(&filesRemaining_lock);

    if (filesRemaining == 0) {  // should never be <, if it is we messed up
      pthread_mutex_unlock(&filesRemaining_lock);
      break;  // mapper is done
    }

    int rc = pthread_create(&id, NULL, (void*)(otp->map), otp->argv[filesRemaining]);
    if (rc) {  // failure creating thread
      printf("Error creating thread.\n");
      exit(1);
    }
    filesRemaining--;
    pthread_mutex_unlock(&filesRemaining_lock);

    pthread_join(id, NULL);
  }
}

/**
 * @brief Returns sorted array
 *
 * @param frontHalf - a pointer to the start of the array
 * @param backHalf - a pointer to the middle of the array
 */
pair* merge(pair* frontHalf, pair* backHalf) {
  pair* newList = (pair*)malloc(sizeof(pair));  // will hold our new sorted list
  if (newList == NULL) {
    printf("Malloc failed\n");
    exit(1);
  }

  pair* currNewList;  // keeps track of where at in NewList
  if (strcmp(frontHalf->key, backHalf->key) <= 0) {  // initialize newList
    newList = frontHalf;
    frontHalf = frontHalf->next;
  } else {
    newList = backHalf;
    backHalf = backHalf->next;
  }
  currNewList = newList;

  while (frontHalf || backHalf) {
    if (!backHalf || (frontHalf && strcmp(frontHalf->key, backHalf->key) <= 0)) {
      currNewList->next = frontHalf;
      currNewList = currNewList->next;
      frontHalf = frontHalf->next;
    } else {  // need to add back half
      currNewList->next = backHalf;
      currNewList = currNewList->next;
      backHalf = backHalf->next;
    }
  }

  return newList;
}

/**
 * @brief Sorts the linked list located at partition_number in internal
 *
 * @param partition_number - the number of the list we're going to sort
 */
pair* mergeSort(pair* headOfList, int lengthOfArr) {
  if (lengthOfArr == 0 || lengthOfArr == 1) {  
    // zero or one element in list, it's sorted (base case)
    return headOfList;
  }

  pair* middleOfList = headOfList;  // iterate until middle of list
  for (int i = 0; i < lengthOfArr / 2; i++) {  // get the middle element
    if (i == lengthOfArr / 2 - 1) {
      pair* tmp = middleOfList;  // shallow copy?
      middleOfList = middleOfList->next;
      tmp->next = NULL;  // should break list in half
      break;
    }
    middleOfList = middleOfList->next;
  }

  if (lengthOfArr % 2 == 1) {
    headOfList = merge(mergeSort(headOfList, lengthOfArr / 2), mergeSort(middleOfList, lengthOfArr / 2 + 1));
  } else {
    headOfList = merge(mergeSort(headOfList, lengthOfArr / 2), mergeSort(middleOfList, lengthOfArr / 2));
  }

  return headOfList;
}

char* MR_Get(char* key, int partition_number) {
  pair* start = internal[partition_number].head;
  if (start == NULL) {
    return NULL;
  }

  if (strcmp(start->key, key) == 0) {
    pair* toRet = malloc(sizeof(pair));
    memcpy(toRet, start, sizeof(pair));
    internal[partition_number].head = internal[partition_number].head->next;
    
    return toRet->value;
  }

  return NULL;
}

void reduceHelper(reducePasser* redPass) {
  while (internal[redPass->partition_number].head && internal[redPass->partition_number].head->key) {
    (redPass->reduce)(internal[redPass->partition_number].head->key, redPass->get, redPass->partition_number);
  }
}

void MR_Run(int argc, char* argv[], Mapper map, int num_mappers, Reducer reduce,
            int num_reducers, Partitioner partition) {
  party = partition;
  init_list(num_reducers);
  int rc;
  pthread_t* mapperArr = malloc(sizeof(pthread_t) * num_mappers);
  if (mapperArr == NULL) {
    printf("Error with malloc\n");
    exit(1);
  }
  filesRemaining = argc - 1;  // we don't have "four" as an argument

  for (int i = 0; i < num_mappers; i++) {
    objToPass otp;
    otp.argv = argv;
    otp.map = map;
    rc = pthread_create(&(mapperArr[i]), NULL, (void*)mapper, &otp);
    if (rc) {
      printf("Error creating thread.\n");
      exit(1);
    }
  }
  for (int i = 0; i < num_mappers; i++) {  // join pthreads after reducers
    pthread_join(mapperArr[i], NULL);
  }
  free(mapperArr);

  for (int i = 0; i < num_reducers; i++) {  // sort each list before reducing
    int tmp_size = 0;
    pair* curr = internal[i].head;
    while (curr) {
      tmp_size++;
      curr = curr->next;
    }

    internal[i].head = mergeSort(internal[i].head, tmp_size);
  }

  pthread_t* reducerArr = malloc(sizeof(pthread_t) * num_reducers);
  if (reducerArr == NULL) {
    printf("Error with malloc\n");
    exit(1);
  }
  reducePasser* redPasses = malloc(sizeof(reducePasser) * num_reducers);
  for (int i = 0; i < num_reducers; i++) {
    redPasses[i].reduce = reduce;
    redPasses[i].get = (void*)MR_Get;
    redPasses[i].partition_number = i;
    rc = pthread_create(&(reducerArr[i]), NULL, (void*)reduceHelper, &(redPasses[i]));
    if (rc) {
      printf("Error making reducer %d\n", i);
      exit(1);
    }
  }
  for (int i = 0; i < num_reducers; i++) {
    pthread_join(reducerArr[i], NULL);
  }
  free(redPasses);
  free(internal);
  free(reducerArr);
  free(bucketBeenAdded);
}
