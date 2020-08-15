# CBQueue Information
Queue which gets callback function with arguments and may execute them by order.
It is simple module (or small lib) written on C. Work with the queue by using the following basic methods:
* Push		O(1),  Worst O(n)  // only when not enough memory
* Execute	O(1),  Worst O(1)
* ChangeSize
* GetSize
* GetDetailedInfo
* SetTimeout (Like in JS)
and etc.

Because the queue can be automatically increased, it is possible to configure the change mode 
(static, up to the certain limit and conditionally unlimited) and the capacity increment vector for the automatic mode.

Also the lib have some macro-flags to configure build for yourself. And the structure itself is split into versions
(so far there are two), it also changes by macro-flags. There are corresponding information lib methods for comparing all this.

In 2 version the lib also have CPP wrapper, in fact, this is a more convenient use case, whan C variant calls.
