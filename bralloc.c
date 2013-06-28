#include <malloc.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#define BRALLOC_PREFIX    "BRALLOC"

#define NAME_DELAY        BRALLOC_PREFIX "_DELAY"
#define NAME_PROBABILITY  BRALLOC_PREFIX "_PROBABILITY"

static void* (*oldMalloc) (size_t, const void*);
static void* (*oldRealloc) (void*, size_t, const void*);

static double failProbability;
static double failStartStamp;


static double currentTimeStamp () {
  struct timeval tv;

  gettimeofday(&tv, 0);

  return (tv.tv_sec) + (tv.tv_usec / 1000000.0);
}

static int mustFail (void) {
  if (failProbability == 0.0) {
    return 0;
  }

  if (failStartStamp > 0.0 && 
      currentTimeStamp() < failStartStamp) {
    return 0;
  }

  if (failProbability * RAND_MAX < rand()) {
    return 0;
  }

  // fail
  return 1;
}

static void* myMalloc (size_t n, const void* x) {
  void* ptr;

  if (mustFail()) {
    return NULL;
  } 

  __malloc_hook = oldMalloc;

  ptr           = malloc(n);

  oldMalloc     = __malloc_hook;   
  __malloc_hook = myMalloc;
  
  return ptr;
}

static void* myRealloc (void* old, size_t n, const void* x) {
  void* ptr;

  if (mustFail()) {
    return NULL;
  } 
  
  __realloc_hook = oldRealloc;

  ptr            = realloc(old, n);

  oldRealloc     = __realloc_hook;    
  __realloc_hook = myRealloc;

  return ptr;
}

static void myInit (void) {
  char* value;

  oldMalloc      = __malloc_hook;
  oldRealloc     = __realloc_hook;

  __malloc_hook  = myMalloc;
  __realloc_hook = myRealloc;
 
  // seed random generator 
  srand(getpid() * 32452843 + time(NULL) * 49979687);
 
  // get failure probability 
  failProbability = 1.0;
  value           = getenv(NAME_PROBABILITY);

  if (value != NULL) {
    double v = strtod(value, NULL);

    if (v >= 0.0 && v <= 1.0) {
      failProbability = v;
    }
  }

  failStartStamp = 0.0;
  value          = getenv(NAME_DELAY);

  if (value != NULL) {
    double v = strtod(value, NULL);

    if (v > 0.0) {
      failStartStamp = currentTimeStamp() + v;
    }
  }
}

#ifdef __MALLOC_HOOK_VOLATILE
void (*__MALLOC_HOOK_VOLATILE __malloc_initialize_hook) (void) = myInit;
#else
void (*__malloc_initialize_hook) (void) = myInit;
#endif

