#ifndef CBQWRAPPER_H
#define CBQWRAPPER_H

#ifdef __cplusplus
extern "C++" {
#endif // __cplusplus

#include "cbqbuildconf.h"
#include "cbqueue.h"
#include "cbqcallbacks.h"

namespace CBQPP {

#include <exception>

    #if CBQ_CUR_VERSION < 2
    #pragma error "Needs CBQueue version 2"
    #endif // CBQ_CUR_VERSION

    #ifndef NO_BASE_CHECK
    #pragma message "it is recommended to set macro NO_BASE_CHECK, when CPP wrapper is used and CBQ is not in lib format"
    #endif // NO_BASE_CHECK

/* Special exception for Queue constructors and assign operators */
class cbqcstr_exception : public std::exception {
private:
    std::string msg;
    int errKey;

public:

    cbqcstr_exception(int key)
    :
        msg(std::string()),
        errKey(key)
    {
        if (!TryBindCstrErrWithMessage(key))
            msg.assign("unknown error");
    }

    virtual ~cbqcstr_exception() = default;

    virtual const char* what(void) const noexcept final override
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
    enum : int {
        CBQ_IN_INITED = 0x51494E49,
        CBQ_IN_FREE = 0x51465245
    };

public:
    explicit Queue(size_t capacity = CBQ_SI_SMALL, int capacityMode = CBQ_SM_LIMIT, size_t maxCapacityLimit = CBQ_SI_MEDIUM, unsigned int initArgsCapacity = 0);
    Queue(const Queue&);
    Queue(Queue&&) noexcept;
    Queue& operator=(const Queue&);
    Queue& operator=(Queue&&);
    ~Queue() noexcept;

    template <typename... Args>
    int Push(QCallback func, Args... arguments) noexcept;
    int Push(QCallback func) noexcept;

    int Execute(int* cb_status = NULL) noexcept;

    template <typename... Args>
    int SetTimeout(QCallback func, clock_t delay, Args... arguments) noexcept;
    int SetTimeout(QCallback func, clock_t delay) noexcept;
    template <typename... Args>
    int SetTimeout(Queue& target, QCallback func, clock_t delay, Args... arguments) noexcept;
    int SetTimeout(Queue& target, QCallback func, clock_t delay) noexcept;

    template <typename... Args>
    int SetTimeoutForSec(QCallback func, clock_t delay, Args... arguments) noexcept;
    int SetTimeoutForSec(QCallback func, clock_t delay) noexcept;
    template <typename... Args>
    int SetTimeoutForSec(Queue& target, QCallback func, clock_t delayInSec, Args... arguments) noexcept;
    int SetTimeoutForSec(Queue& target, QCallback func, clock_t delayInSec) noexcept;

    size_t Size(void) const noexcept;

private:
    template <typename T> CBQArg_t CBQ_argConvert__(T val) noexcept;

};

inline Queue::Queue(size_t capacity, int capacityMode, size_t maxCapacityLimit, unsigned int initArgsCapacity)
{
    int err = CBQ_QueueInit(&cbq, capacity, capacityMode, maxCapacityLimit, initArgsCapacity);
    if (err)
        throw(cbqcstr_exception(err));
}

inline Queue::Queue(const Queue& other)
{
    int err = CBQ_QueueCopy(&this->cbq, &other.cbq);
    if (err)
        throw(cbqcstr_exception(err));
}

inline Queue::Queue(Queue&& other) noexcept
{
    this->cbq = other.cbq;
    other.cbq.coArr = NULL;   // C-like const
    other.cbq.initSt = CBQ_IN_FREE; // for QueueFree
}

inline Queue& Queue::operator=(const Queue& other)
{
    if (this == &other)
        return *this;

    int err = CBQ_QueueFree(&this->cbq);
    if (err)
        throw(cbqcstr_exception(err));

    err = CBQ_QueueCopy(&this->cbq, &other.cbq);
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

    this->cbq = other.cbq;
    other.cbq.coArr = NULL;
    other.cbq.initSt = CBQ_IN_FREE;
}

inline Queue::~Queue() noexcept
{
    CBQ_QueueFree(&this->cbq);
}

/* Type filters and union setters */
template <typename T> inline CBQArg_t Queue::CBQ_argConvert__(T) noexcept {
    static_assert( static_cast<signed int>(sizeof(T)) < -1, "Unknown CB argument, check CBQArg_t declaration");
    return {0};
}

#ifdef NO_FIX_ARGTYPES
// unsigned
template <> inline CBQArg_t Queue::CBQ_argConvert__<unsigned char>(unsigned char val) noexcept                  {CBQArg_t arg; arg.utiVar = val; return arg;}
template <> inline CBQArg_t Queue::CBQ_argConvert__<unsigned short>(unsigned short val) noexcept                {CBQArg_t arg; arg.usiVar = val; return arg;}
template <> inline CBQArg_t Queue::CBQ_argConvert__<unsigned int>(unsigned int val) noexcept                    {CBQArg_t arg; arg.uiVar = val; return arg;}
template <> inline CBQArg_t Queue::CBQ_argConvert__<unsigned long long>(unsigned long long val) noexcept        {CBQArg_t arg; arg.ulliVar = val; return arg;}
// signed
template <> inline CBQArg_t Queue::CBQ_argConvert__<signed char>(signed char val) noexcept                      {CBQArg_t arg; arg.tiVar = val; return arg;}
template <> inline CBQArg_t Queue::CBQ_argConvert__<signed short>(signed short val) noexcept                    {CBQArg_t arg; arg.siVar = val; return arg;}
template <> inline CBQArg_t Queue::CBQ_argConvert__<signed int>(signed int val) noexcept                        {CBQArg_t arg; arg.iVar = val; return arg;}
template <> inline CBQArg_t Queue::CBQ_argConvert__<signed long long>(signed long long val) noexcept            {CBQArg_t arg; arg.lliVar = val; return arg;}
#else
// unsigned
template <> inline CBQArg_t Queue::CBQ_argConvert__<uint8_t>(uint8_t val) noexcept                              {CBQArg_t arg; arg.utiVar = val; return arg;}
template <> inline CBQArg_t Queue::CBQ_argConvert__<uint16_t>(uint16_t val) noexcept                            {CBQArg_t arg; arg.usiVar = val; return arg;}
template <> inline CBQArg_t Queue::CBQ_argConvert__<uint32_t>(uint32_t val) noexcept                            {CBQArg_t arg; arg.uiVar = val; return arg;}
template <> inline CBQArg_t Queue::CBQ_argConvert__<uint64_t>(uint64_t val) noexcept                            {CBQArg_t arg; arg.ulliVar = val; return arg;}
// signed
template <> inline CBQArg_t Queue::CBQ_argConvert__<int8_t>(int8_t val) noexcept                                {CBQArg_t arg; arg.tiVar = val; return arg;}
template <> inline CBQArg_t Queue::CBQ_argConvert__<int16_t>(int16_t val) noexcept                              {CBQArg_t arg; arg.siVar = val; return arg;}
template <> inline CBQArg_t Queue::CBQ_argConvert__<int32_t>(int32_t val) noexcept                              {CBQArg_t arg; arg.iVar = val; return arg;}
template <> inline CBQArg_t Queue::CBQ_argConvert__<int64_t>(int64_t val) noexcept                              {CBQArg_t arg; arg.lliVar = val; return arg;}
#endif // NO_FIX_ARGTYPES
template <> inline CBQArg_t Queue::CBQ_argConvert__<unsigned long>(unsigned long val) noexcept                  {CBQArg_t arg; arg.uliVar = val; return arg;}
template <> inline CBQArg_t Queue::CBQ_argConvert__<signed long>(signed long val) noexcept                      {CBQArg_t arg; arg.liVar = val; return arg;}
// float
template <> inline CBQArg_t Queue::CBQ_argConvert__<float>(float val) noexcept                                  {CBQArg_t arg; arg.flVar = val; return arg;}
template <> inline CBQArg_t Queue::CBQ_argConvert__<double>(double val) noexcept                                {CBQArg_t arg; arg.dVar = val; return arg;}
// symbols
template <> inline CBQArg_t Queue::CBQ_argConvert__<char>(char val) noexcept                                    {CBQArg_t arg; arg.cVar = val; return arg;}
template <> inline CBQArg_t Queue::CBQ_argConvert__<const char*>(const char* val) noexcept                      {CBQArg_t arg; arg.sVar = const_cast<char*>(val); return arg;}
template <> inline CBQArg_t Queue::CBQ_argConvert__<char*>(char* val) noexcept                                  {CBQArg_t arg; arg.sVar = val; return arg;}
// pointers
template <> inline CBQArg_t Queue::CBQ_argConvert__<void*>(void* val) noexcept                                  {CBQArg_t arg; arg.pVar = val; return arg;}
template <> inline CBQArg_t Queue::CBQ_argConvert__<CBQueue_t*>(CBQueue_t* val) noexcept                        {CBQArg_t arg; arg.qVar = val; return arg;}
template <> inline CBQArg_t Queue::CBQ_argConvert__<QCallback>(QCallback val) noexcept                          {CBQArg_t arg; arg.fVar = val; return arg;}

// ...you didn't see it, okay?

template <typename... Args>
inline int Queue::Push(QCallback func, Args... arguments) noexcept
{
    return CBQ_Push(&this->cbq, func, 0, CBQ_NO_VPARAMS, sizeof...(arguments), CBQ_argConvert__<Args>(arguments)...);
}

inline int Queue::Push(QCallback func) noexcept
{
    return CBQ_PushVoid(&this->cbq, func);
}

inline int Queue::Execute(int* cb_status) noexcept
{
    return CBQ_Exec(&this->cbq, cb_status);
}


template <typename... Args>
inline int Queue::SetTimeout(QCallback func, clock_t delay, Args... arguments) noexcept
{
    CBQArg_t params[] = {CBQ_argConvert__<Args>(arguments)...};
    return CBQ_SetTimeout(&this->cbq, delay, 0, &this->cbq, func, sizeof...(arguments), params);
}

inline int Queue::SetTimeout(QCallback func, clock_t delay) noexcept
{
    return CBQ_SetTimeout(&this->cbq, delay, 0, &this->cbq, func, 0,  CBQ_NO_VPARAMS);
}

template <typename... Args>
inline int Queue::SetTimeout(Queue& target, QCallback func, clock_t delay, Args... arguments) noexcept
{
    CBQArg_t params[] = {CBQ_argConvert__<Args>(arguments)...};
    return CBQ_SetTimeout(&this->cbq, delay, 0, &target.cbq, func, sizeof...(arguments), params);
}

inline int Queue::SetTimeout(Queue& target, QCallback func, clock_t delay) noexcept
{
    return CBQ_SetTimeout(&this->cbq, delay, 0, &target.cbq, func, 0,  CBQ_NO_VPARAMS);
}

template <typename... Args>
inline int Queue::SetTimeoutForSec(QCallback func, clock_t delayInSec, Args... arguments) noexcept
{
    CBQArg_t params[] = {CBQ_argConvert__<Args>(arguments)...};
    return CBQ_SetTimeout(&this->cbq, delayInSec, 1, &this->cbq, func, sizeof...(arguments), params);
}

inline int Queue::SetTimeoutForSec(QCallback func, clock_t delayInSec) noexcept
{
    return CBQ_SetTimeout(&this->cbq, delayInSec, 1, &this->cbq, func, 0, CBQ_NO_VPARAMS);
}

template <typename... Args>
inline int Queue::SetTimeoutForSec(Queue& target, QCallback func, clock_t delayInSec, Args... arguments) noexcept
{
    CBQArg_t params[] = {CBQ_argConvert__<Args>(arguments)...};
    return CBQ_SetTimeout(&this->cbq, delayInSec, 1, &target.cbq, func, sizeof...(arguments), params);
}

inline int Queue::SetTimeoutForSec(Queue& target, QCallback func, clock_t delayInSec) noexcept
{
    return CBQ_SetTimeout(&this->cbq, delayInSec, 1, &target.cbq, func, 0,  CBQ_NO_VPARAMS);
}


inline size_t Queue::Size(void) const noexcept
{
    size_t size;
    CBQ_GetSize(&this->cbq, &size);
    return size;
}

}   // CBQPP namespace

#ifdef __cplusplus
}   // C++ extern
#endif // __cplusplus

#endif
