bralloc
=======

an intentionally failing malloc wrapper, useful for checking programs in low or out-of-memory situations.

bralloc is inspired by [failmalloc](http://www.nongnu.org/failmalloc/), which I found to be not working in the environment I tried to use it in. 

building bralloc
================

Download the repo:

    git clone https://github.com/jsteemann/bralloc
    cd bralloc

Then run configure and make

    ./configure
    make

This will build the bralloc shared object in some subdirectory, probably `.libs`.

You may also want to install the shared object but this is not required:

    sudo make install

The shared object is going to be installed in a system-dependent place, probably `/usr/lib/`, `/usr/local/lib/`, `/usr/lib64/` etc.

usage
=====

bralloc can be invoked dynamically via `LD_PRELOAD`ing:

    LD_PRELOAD=.libs/libbralloc.so <command to run>

When invoked, it wraps calls to the system's *malloc* and *realloc* functions with its own functions.

bralloc will decide in its wrappers whether it will return a NULL pointer (intentionally fail to allocate new memory) or pass the memory allocation request to the actual system allocator.

At what time and at which frequency there will be intentional allocation failures is controllable by the environment variables:
* `BRALLOC_DELAY`: the startup delay (in seconds) after which the first intentional failure will occur. The default value is 0.0.
* `BRALLOC_MINIMUM`: the minimum byte size that intentional failures are triggered for. mallocs/reallocs operations with smaller sizes will be directed to the standard allocators.
* `BRALLOC_PROBABILITY`: the probability (in range 0...1) at which failures occur after the startup delay. The default value is 0.01 (1%).

bralloc wraps calls to *malloc* and *realloc*. This should include calls to *calloc*. Calls to *free* are not wrapped and are processed normally.

examples
========

Run `ls -alh` but fail at every 20th memory allocation request (5% failure probability):

    BRALLOC_DELAY=0 BRALLOC_PROBABILITY=0.05 LD_PRELOAD=.libs/libbralloc.so ls -alh

Run `curl` with a failure probability of 50% for every malloc allocation >= 4096 bytes:

    BRALLOC_PROBABILITY=0.5 BRALLOC_MINIMUM=4096 LD_PRELOAD=/usr/lib/libbralloc.so curl -X GET https://github.com 

Run `mysql` with a failure probability of 20% after 60 second startup delay:

    BRALLOC_DELAY=60 BRALLOC_PROBABILITY=0.2 LD_PRELOAD=/usr/local/lib/libbralloc.so mysql ...

caveats
=======

bralloc uses the libc malloc wrappers, which have been marked as *deprecated*. However it may still work.

bralloc doesn't care about thread-safety and might itself lead to issues. Still it may be helpful for testing your program's behavior in the face of low or out-of-memory situations.

It probably won't work together with other memory subsystem interceptors or wrappers.

Seems to work on Ubuntu, but the status on other platforms is unknown. For sure it doesn't work on Windows.

:squirrel:
