#ifndef CBQWRAPPER_H
#define CBQWRAPPER_H

#ifndef __cplusplus
extern "C++" {
    namespace CBQPP {
#endif // __cplusplus

#include <exception>
#include "cbqbuildconf.h"

    #if CBQ_CUR_VERSION < 2
    #pragma error "Needs CBQueue version 2"
    #endif // CBQ_CUR_VERSION

    #ifndef NO_BASE_CHECK
    #pragma message "it is recommended to set macro NO_BASE_CHECK, when CPP wrapper is used"
    #endif // NO_BASE_CHECK

#include "cbqueue.h"

/* Special exception for Queue constructors and assign operators */
class cbqcstr_exception : public exception {
private:
    std::string msg;
    int errKey;

public:

    cbqcstr_exception(int key)
    :
        msg(std::string()),
        errKey(key)
    {
        if !(TryBindCstrErrWithMessage(key))
            msg.assign("unknown error");
    }

    virtual ~cbqcstr_exception() = default;

    virtual const char* what(void) final override
    {
        return msg.c_str();
    }

    inline int GetErrorKey(void)
    {
        return errKey;
    }

private:

    bool TryBindCstrErrWithMessage(int errKey)
    {
        bool unknown = false;
        switch (errKey) {

        /* After move object by rvalue, the source object
         * now unavailable for use. Returning a rvalue link or lvalue
         * to rvalue casting better be avoided.
         * Also its not throws when macro NO_BASE_CHECK is set.
         */
        case CBQ_ERR_NOT_INITED:
            msg.assign("Atempt to use moved instance");
            break;

        case CBQ_ERR_ARG_OUT_OF_RANGE:
            msg.assign("Wrong range of capacity in arguments");
            break;

        case CBQ_ERR_MEM_ALLOC_FAILED:
            msg.assign("Cannot allocate memory for queue");
            break;

        case CBQ_ERR_MEM_BUT_RESTORED:
            msg.assign("Allocate mem failed, but restored");
            break;

        case CBQ_ERR_IS_BUSY:
            msg.assign("Cstr/assign/changing size operations with Queue instance in own callbacks is forbid");
            break;

        default:
            unknown = true;
        };

        return unknown;
    }
}; // cbqcstr_exception class

/* Main class wrapper */
class Queue {
private:
    CBQueue_t cbq;

public:
    explicit Queue(size_t capacity = CBQ_SI_SMALL, capacityMode = CBQ_SM_LIMIT, size_t maxCapacityLimit = CBQ_SI_MEDIUM, initArgsCapacity = 0);
    Queue(const Queue&);
    Queue(Queue&&);
    Queue& operator=(const Queue&);

private:
    ~Queue();
};

inline Queue::Queue(size_t capacity, capacityMode, size_t maxCapacityLimit, initArgsCapacity)
{
    int err = CBQ_QueueInit(&cbq, capacity, capacityMode, maxCapacityLimit, initArgsCapacity);
    if (err)
        throw(cbqcstr_exception(err));
}

inline Queue::Queue(const Queue& other)
{
    int err = CBQ_QueueCopy(&this->cbq, &other->cbq);
    if (err)
        throw(cbqcstr_exception(err));
}

inline Queue::Queue(Queue&& other) noexcept
{
    this->cbq = other->cbq;
    other->cbq.coArr = NULL;   // C-like const
    other->cbq.initSt = CBQ_IN_FREE; // for QueueFree
}

inline Queue& Queue::operator=(const Queue& other)
{
    if (this == &other)
        return *this;

    int err = CBQ_QueueFree(&this->cbq);
    if (err)
        throw(cbqcstr_exception(err));

    err = CBQ_QueueCopy(&this->cbq, &other->cbq);
    if (err)
        throw(cbqcstr_exception(err));

    return *this;
}

inline Queue& Queue::operator=(Queue&& other)
{
    if (this == &other)
        return *this;

    int err = CBQ_QueueFree(&this->cbq);
    if (err)
        throw(cbqcstr_exception(err));

    this->cbq = other->cbq;
    other->cbq.coArr = NULL;
    other->cbq.initSt = CBQ_IN_FREE;
}

inline Queue::~Queue()
{
    CBQ_QueueFree(&this->cbq);
}

#ifndef __cplusplus
    }   // CBQPP namespace
}   // C++ extern
#endif // __cplusplus

#endif
