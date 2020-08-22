# CBQueue Information
Queue which gets callback function with arguments and may execute them by order.
It is simple module (or small lib) written on C. Work with the queue by using the following basic methods:
* *Push*
  * O(1),  Worst - O(n)   (only when not enough memory)
* *Execute*
  * O(1),  Worst - O(1)
* *ChangeSize*
* *GetDetailedInfo*
* *SetTimeout* (Like in JS)
* and more else...

Please, check examples of using in C/C++ (while there are many examples in cbqtest.c).

Since the library was made with a bias towards safe use, there is a restriction
inside direct calls of callbacks of the same queue:
 * user changing the capacity of the queue
 * changing the capacity modes of the queue
 * arguments equalizing, changing argument init size
 * skipping, transfering, copying into queue
 * executing callbacks.
There is no restriction on adding callbacks during call in same queue.

The restriction can be removed at your own risk through a macro *NO_EXCEPTIONS_OF_BUSY*

Because the queue can be automatically increased, it is possible to configure the change mode 
(static, up to the certain limit and conditionally unlimited) and the capacity increment vector for the automatic mode.

Also the lib have some macro-flags to configure build for yourself. And the structure itself is split into versions
(so far there are two), it also changes by macro-flags. There are corresponding information lib methods for comparing all this.
Check *cbqbuildconf.h* for detail information.

In 2 version the lib also have C++ wrapper, in fact, this is a more convenient use case, whan C variant calls.
