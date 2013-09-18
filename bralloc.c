#include <errno.h>
#include <malloc.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#define BRALLOC_PREFIX    "BRALLOC"

#define NAME_DELAY        BRALLOC_PREFIX "_DELAY"
#define NAME_MIN_SIZE     BRALLOC_PREFIX "_MINIMUM"
#define NAME_PROBABILITY  BRALLOC_PREFIX "_PROBABILITY"


static void* (*oldMalloc) (size_t, const void*);
static void* (*oldRealloc) (void*, size_t, const void*);

static double failDelay;
static size_t failMinSize;
static double failProbability;
static double failStartStamp;

pthread_mutex_t lock;


static double currentTimeStamp (void) {
  struct timeval tv;

  gettimeofday(&tv, 0);

  return (tv.tv_sec) + (tv.tv_usec / 1000000.0);
}


static int mustFail (size_t n) {
  // size check
  if (failMinSize > 0 && n < failMinSize) {
    // requested size is smaller than minimum size
    return 0;
  }

  if (failProbability == 0.0) {
    // failure probability is 0
    return 0;
  }

  if (failStartStamp > 0.0 && 
      currentTimeStamp() < failStartStamp) {
    // we are still before the start of our fail series
    return 0;
  }

  if (failProbability < 1.0 && 
      failProbability * RAND_MAX < rand()) {
    // probability not high enough
    return 0;
  }

  // fail
  return 1;
}


static void* myMalloc (size_t n, const void* x) {
  void* ptr;

  pthread_mutex_lock(&lock);
  __malloc_hook   = oldMalloc;

  if (n > 0 && mustFail(n)) {
    oldMalloc     = __malloc_hook;   
    __malloc_hook = myMalloc;
  
    // indicate error
    errno         = ENOMEM;
    pthread_mutex_unlock(&lock);

    return NULL;
  } 

  ptr             = malloc(n);

  oldMalloc       = __malloc_hook;   
  __malloc_hook   = myMalloc;
    
  pthread_mutex_unlock(&lock);
  
  return ptr;
}


static void* myRealloc (void* old, size_t n, const void* x) {
  void* ptr;

  pthread_mutex_lock(&lock);
  __realloc_hook   = oldRealloc;

  if (n > 0 && mustFail(n)) {
    oldMalloc      = __malloc_hook;   
    __malloc_hook  = myMalloc;
  
    // indicate error
    errno          = ENOMEM;
    pthread_mutex_unlock(&lock);

    return NULL;
  } 
  
  ptr              = realloc(old, n);

  oldRealloc       = __realloc_hook;    
  __realloc_hook   = myRealloc;
  
  pthread_mutex_unlock(&lock);

  return ptr;
}


static void myInit (void) {
  char* value;
  pid_t pid;

  pthread_mutex_init(&lock, NULL);

  oldMalloc       = __malloc_hook;
  oldRealloc      = __realloc_hook;

  __malloc_hook   = myMalloc;
  __realloc_hook  = myRealloc;
 
  pid             = getpid();
  failDelay       = 0.0;
  failMinSize     = 0;
  failProbability = 0.1;
  failStartStamp  = 0.0;

  // seed random generator
  srand(pid * 32452843 + time(NULL) * 49979687);
 
  // get failure probability 
  value = getenv(NAME_PROBABILITY);

  if (value != NULL) {
    double v = strtod(value, NULL);

    if (v >= 0.0 && v <= 1.0) {
      failProbability = v;
    }
  }

  // get startup delay
  value = getenv(NAME_DELAY);

  if (value != NULL) {
    double v = strtod(value, NULL);

    if (v > 0.0) {
      failDelay = v;
      failStartStamp = currentTimeStamp() + failDelay;
    }
  }
 
  // get minimum size for failures 
  value = getenv(NAME_MIN_SIZE);

  if (value != NULL) {
    unsigned long long v = strtoull(value, NULL, 10);

    if (v > 0) {
      failMinSize = (size_t) v;
    }
  }

  fprintf(stderr,
          "WARNING: program execution with bralloc wrapper.\n"
          "================================================\n"
          "process id:          %llu\n"
          "failure probability: %0.3f %%\n"
          "failure delay:       %0.1f s\n"
          "failure min size:    %llu\n\n",
          (unsigned long long) pid,
          100.0 * failProbability, 
          failDelay,
          (unsigned long long) failMinSize);
}


#ifdef __MALLOC_HOOK_VOLATILE
void (*__MALLOC_HOOK_VOLATILE __malloc_initialize_hook) (void) = myInit;
#else
void (*__malloc_initialize_hook) (void) = myInit;
#endif

