#pragma once

/* C++ wrapper, which is more comfortable for using CBQ library */

#include "cbqbuildconf.h"
#include "cbqueue.h"
#include "cbqcallbacks.h"
#include "cbqversion.h"
#include <exception>
#include <string>

namespace CBQPP {

    #if !defined(__cplusplus) || __cplusplus < 201103L
    #error "Needs c++11 standart version"
    #endif

    #if CBQ_CUR_VERSION < 2
    #error "Needs CBQueue version 2"
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

public:
    explicit Queue(size_t capacity = CBQ_SI_SMALL, CBQ_CapacityModes capacityMode = CBQ_SM_LIMIT, size_t maxCapacityLimit = CBQ_SI_BIG, unsigned int initArgsCapacity = 0);
    Queue(const Queue&);
    Queue(Queue&&) noexcept;
    Queue& operator=(const Queue&);
    Queue& operator=(Queue&&);
    ~Queue() noexcept;

    int Clear(void) noexcept;
    int Concat(const Queue& source) noexcept;
    int Transfer(Queue& other, size_t count, bool considerCapacity = false, bool considerSourceSize = true) noexcept;
    int Skip(size_t count, bool atBack = false, bool considerSize = true) noexcept;

    template <typename... Args>
    int Push(QCallback func, Args... arguments) noexcept;
    int Push(QCallback func) noexcept;
    template <typename FuncT, typename... Args>
    int Push(FuncT func, Args... arguments) noexcept;

    template <typename... Args>
    int SetTimeout(QCallback func, clock_t delay, Args... arguments) noexcept;
    template <typename FuncT, typename... Args>
    int SetTimeout(FuncT func, clock_t delay, Args... arguments) noexcept;
    template <typename... Args>
    int SetTimeout(Queue& target, QCallback func, clock_t delay, Args... arguments) noexcept;
    template <typename FuncT, typename... Args>
    int SetTimeout(Queue& target, FuncT func, clock_t delay, Args... arguments) noexcept;

    template <typename... Args>
    int SetTimeoutForSec(QCallback func, clock_t delay, Args... arguments) noexcept;
    template <typename FuncT, typename... Args>
    int SetTimeoutForSec(FuncT func, clock_t delay, Args... arguments) noexcept;
    template <typename... Args>
    int SetTimeoutForSec(Queue& target, QCallback func, clock_t delayInSec, Args... arguments) noexcept;
    template <typename FuncT, typename... Args>
    int SetTimeoutForSec(Queue& target, FuncT func, clock_t delayInSec, Args... arguments) noexcept;

    int Execute(int* cb_status = NULL) noexcept;

    size_t Size(void) const noexcept;
    size_t Capacity(void) const noexcept;
    size_t CapacityInBytes(void) const noexcept;
    bool IsEmpty(void) const noexcept;
    bool IsFull(void) const noexcept;
    void DetailInfo
    (size_t* size = NULL, size_t* capacity = NULL, int* incCapMode =  NULL, size_t* maxCapLimit = NULL, size_t* sizeInBytes = NULL)
    const noexcept;

    int IncreaseCapacity(void) noexcept;
    int DecreaseCapacity(void) noexcept;
    int ChangeCapacity(size_t newSize, bool considerLimits = true) noexcept;
    int ChangeCapacity(bool increase) noexcept;

    int ChangeIncreaseCapacityMode(CBQ_CapacityModes newMode, size_t newLimit = CBQ_SI_MEDIUM, bool adaptLimit = true, bool tryToAdaptCapacity = true) noexcept;
    int EqualizeArgumentsCapacity(unsigned int requiredCapacity, bool skipNoneModifiable = true) noexcept;
    int ChangeInitArgumentsCapacity(unsigned int newCapacity) noexcept;

    static int GetVerIndex(void);
    static bool CheckVerIndexByFlag(CBQ_VI fInfoType);
    static int GetDifferencesVerIndexMask(int comparedVerId);
    static int GetAvaliableFlagsRange(void);
    static bool IsCustomisedVersion(void);

private:
    template <typename T> static CBQArg_t CBQ_convertToArg__(T val) noexcept;
    template <typename T> static T CBQ_convertToVal__(CBQArg_t arg) noexcept;
    template <typename FuncT, typename... ArgsT> static CBQArg_t CBQ_packCustomCB__(FuncT customCB) noexcept;
    template <typename... ArgsT> static int CBQ_invokeCustomCB__(int argc, CBQArg_t* argv) noexcept;
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++" // no need for members init list

inline Queue::Queue(size_t capacity, CBQ_CapacityModes capacityMode, size_t maxCapacityLimit, unsigned int initArgsCapacity)
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
    CBQ_QueueCorrectMove(&this->cbq, &other.cbq);
}

#pragma GCC diagnostic pop

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
    CBQ_QueueCorrectMove(&this->cbq, &other.cbq);
    return *this;
}

inline Queue::~Queue() noexcept
{
    CBQ_QueueFree(&this->cbq);
}

inline int Queue::Clear(void) noexcept
{
    return CBQ_Clear(&this->cbq);
}

inline int Queue::Concat(const Queue& source) noexcept
{
    return CBQ_QueueConcat(&this->cbq, &source.cbq);
}

inline int Queue::Transfer(Queue& source, size_t count, bool considerCapacity, bool considerSourceSize) noexcept
{
    return CBQ_QueueTransfer(&this->cbq, &source.cbq, count, static_cast<int>(considerSourceSize), static_cast<int>(considerCapacity));
}

inline int Queue::Skip(size_t count, bool atBack, bool considerSize) noexcept
{
    return CBQ_Skip(&this->cbq, count, static_cast<int>(considerSize), static_cast<int>(atBack));
}

/* Type filters and union setters */
template <typename T> inline CBQArg_t Queue::CBQ_convertToArg__(T) noexcept {
    static_assert( static_cast<signed int>(sizeof(T)) < -1, "Unknown CB argument, check CBQArg_t declaration");
    return {0};
}

#ifdef NO_FIX_ARGTYPES
// unsigned
template <> inline CBQArg_t Queue::CBQ_convertToArg__<unsigned char>(unsigned char val) noexcept                  {CBQArg_t arg; arg.utiVar = val; return arg; }
template <> inline CBQArg_t Queue::CBQ_convertToArg__<unsigned short>(unsigned short val) noexcept                {CBQArg_t arg; arg.usiVar = val; return arg; }
template <> inline CBQArg_t Queue::CBQ_convertToArg__<unsigned int>(unsigned int val) noexcept                    {CBQArg_t arg; arg.uiVar = val; return arg;  }
template <> inline CBQArg_t Queue::CBQ_convertToArg__<unsigned long long>(unsigned long long val) noexcept        {CBQArg_t arg; arg.ulliVar = val; return arg;}
// signed
template <> inline CBQArg_t Queue::CBQ_convertToArg__<signed char>(signed char val) noexcept                      {CBQArg_t arg; arg.tiVar = val; return arg; }
template <> inline CBQArg_t Queue::CBQ_convertToArg__<signed short>(signed short val) noexcept                    {CBQArg_t arg; arg.siVar = val; return arg; }
template <> inline CBQArg_t Queue::CBQ_convertToArg__<signed int>(signed int val) noexcept                        {CBQArg_t arg; arg.iVar = val; return arg;  }
template <> inline CBQArg_t Queue::CBQ_convertToArg__<signed long long>(signed long long val) noexcept            {CBQArg_t arg; arg.lliVar = val; return arg;}
#else
// unsigned
template <> inline CBQArg_t Queue::CBQ_convertToArg__<uint8_t>(uint8_t val) noexcept                              {CBQArg_t arg; arg.utiVar = val; return arg; }
template <> inline CBQArg_t Queue::CBQ_convertToArg__<uint16_t>(uint16_t val) noexcept                            {CBQArg_t arg; arg.usiVar = val; return arg; }
template <> inline CBQArg_t Queue::CBQ_convertToArg__<uint32_t>(uint32_t val) noexcept                            {CBQArg_t arg; arg.uiVar = val; return arg;  }
template <> inline CBQArg_t Queue::CBQ_convertToArg__<uint64_t>(uint64_t val) noexcept                            {CBQArg_t arg; arg.ulliVar = val; return arg;}
// signed
template <> inline CBQArg_t Queue::CBQ_convertToArg__<int8_t>(int8_t val) noexcept                                {CBQArg_t arg; arg.tiVar = val; return arg; }
template <> inline CBQArg_t Queue::CBQ_convertToArg__<int16_t>(int16_t val) noexcept                              {CBQArg_t arg; arg.siVar = val; return arg; }
template <> inline CBQArg_t Queue::CBQ_convertToArg__<int32_t>(int32_t val) noexcept                              {CBQArg_t arg; arg.iVar = val; return arg;  }
template <> inline CBQArg_t Queue::CBQ_convertToArg__<int64_t>(int64_t val) noexcept                              {CBQArg_t arg; arg.lliVar = val; return arg;}
#endif // NO_FIX_ARGTYPES
template <> inline CBQArg_t Queue::CBQ_convertToArg__<unsigned long>(unsigned long val) noexcept                  {CBQArg_t arg; arg.uliVar = val; return arg;}
template <> inline CBQArg_t Queue::CBQ_convertToArg__<signed long>(signed long val) noexcept                      {CBQArg_t arg; arg.liVar = val; return arg; }
// float
template <> inline CBQArg_t Queue::CBQ_convertToArg__<float>(float val) noexcept                                  {CBQArg_t arg; arg.flVar = val; return arg;}
template <> inline CBQArg_t Queue::CBQ_convertToArg__<double>(double val) noexcept                                {CBQArg_t arg; arg.dVar = val; return arg; }
// symbols
template <> inline CBQArg_t Queue::CBQ_convertToArg__<char>(char val) noexcept                                    {CBQArg_t arg; arg.cVar = val; return arg;}
template <> inline CBQArg_t Queue::CBQ_convertToArg__<const char*>(const char* val) noexcept                      {CBQArg_t arg; arg.sVar = const_cast<char*>(val); return arg;}
template <> inline CBQArg_t Queue::CBQ_convertToArg__<char*>(char* val) noexcept                                  {CBQArg_t arg; arg.sVar = val; return arg;}
// pointers
template <> inline CBQArg_t Queue::CBQ_convertToArg__<void*>(void* val) noexcept                                  {CBQArg_t arg; arg.pVar = val; return arg;}
template <> inline CBQArg_t Queue::CBQ_convertToArg__<CBQueue_t*>(CBQueue_t* val) noexcept                        {CBQArg_t arg; arg.qVar = val; return arg;}
template <> inline CBQArg_t Queue::CBQ_convertToArg__<QCallback>(QCallback val) noexcept                          {CBQArg_t arg; arg.fVar = val; return arg;}

template <typename... Args>
inline int Queue::Push(QCallback func, Args... arguments) noexcept
{
    return CBQ_Push(&this->cbq, func, 0, CBQ_NO_VPARAMS, sizeof...(arguments), CBQ_convertToArg__<Args>(arguments)...);
}

inline int Queue::Push(QCallback func) noexcept
{
    return CBQ_PushVoid(&this->cbq, func);
}


template <typename T> inline T Queue::CBQ_convertToVal__(CBQArg_t val) noexcept
{
    static_assert( static_cast<signed int>(sizeof(T)) < -1, "Unsupported CB argument, check CBQArg_t declaration");
}

#ifndef NO_FIX_ARGTYPES
template <> inline signed char Queue::CBQ_convertToVal__<signed char>             (CBQArg_t val) noexcept        {return val.tiVar; }
template <> inline signed short Queue::CBQ_convertToVal__<signed short>           (CBQArg_t val) noexcept        {return val.siVar; }
template <> inline signed int Queue::CBQ_convertToVal__<signed int>               (CBQArg_t val) noexcept        {return val.iVar;  }
template <> inline signed long long Queue::CBQ_convertToVal__<signed long long>   (CBQArg_t val) noexcept        {return val.lliVar;}
template <> inline unsigned char Queue::CBQ_convertToVal__<unsigned char>             (CBQArg_t val) noexcept        {return val.utiVar; }
template <> inline unsigned short Queue::CBQ_convertToVal__<unsigned short>           (CBQArg_t val) noexcept        {return val.usiVar; }
template <> inline unsigned int Queue::CBQ_convertToVal__<unsigned int>               (CBQArg_t val) noexcept        {return val.uiVar;  }
template <> inline unsigned long long Queue::CBQ_convertToVal__<unsigned long long>   (CBQArg_t val) noexcept        {return val.ulliVar;}
#else
template <> inline int8_t Queue::CBQ_convertToVal__<int8_t>             (CBQArg_t val) noexcept        {return val.tiVar; }
template <> inline int16_t Queue::CBQ_convertToVal__<int16_t>           (CBQArg_t val) noexcept        {return val.siVar; }
template <> inline int32_t Queue::CBQ_convertToVal__<int32_t>           (CBQArg_t val) noexcept        {return val.iVar;  }
template <> inline int64_t Queue::CBQ_convertToVal__<int64_t>           (CBQArg_t val) noexcept        {return val.lliVar;}
template <> inline uint8_t Queue::CBQ_convertToVal__<uint8_t>             (CBQArg_t val) noexcept        {return val.utiVar; }
template <> inline uint16_t Queue::CBQ_convertToVal__<uint16_t>           (CBQArg_t val) noexcept        {return val.usiVar; }
template <> inline uint32_t Queue::CBQ_convertToVal__<uint32_t>           (CBQArg_t val) noexcept        {return val.uiVar;  }
template <> inline uint64_t Queue::CBQ_convertToVal__<uint64_t>           (CBQArg_t val) noexcept        {return val.ulliVar;}
#endif
template <> inline signed long Queue::CBQ_convertToVal__<signed long>   (CBQArg_t val) noexcept             {return val.liVar; }
template <> inline unsigned long Queue::CBQ_convertToVal__<unsigned long>   (CBQArg_t val) noexcept         {return val.uliVar;}
template <> inline float Queue::CBQ_convertToVal__<float>   (CBQArg_t val) noexcept                         {return val.flVar; }
template <> inline double Queue::CBQ_convertToVal__<double>   (CBQArg_t val) noexcept                       {return val.dVar;  }
template <> inline char Queue::CBQ_convertToVal__<char>(CBQArg_t val) noexcept                              {return val.cVar;  }
template <> inline char* Queue::CBQ_convertToVal__<char*>(CBQArg_t val) noexcept                            {return val.sVar;  }
template <> inline const char* Queue::CBQ_convertToVal__<const char*>(CBQArg_t val) noexcept                {return const_cast<const char*>(val.sVar);}
template <> inline void* Queue::CBQ_convertToVal__<void*>(CBQArg_t val) noexcept                            {return val.pVar;  }
template <> inline CBQueue_t* Queue::CBQ_convertToVal__<CBQueue_t*>(CBQArg_t val) noexcept                  {return val.qVar;  }
template <> inline QCallback Queue::CBQ_convertToVal__<QCallback>(CBQArg_t val) noexcept                    {return val.fVar;  }

template<typename... ArgsT>
inline int Queue::CBQ_invokeCustomCB__(int argc, CBQArg_t* argv) noexcept
{
    constexpr unsigned int CUSTOM_CB_ID = 0;
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wsequence-point"
    return static_cast<int>(reinterpret_cast<int (*)(ArgsT...)>(argv[CUSTOM_CB_ID].pVar)( CBQ_convertToVal__<ArgsT>(argv[--argc])...));
    #pragma GCC diagnostic pop
}

template <typename FuncT, typename... ArgsT>
inline CBQArg_t Queue::CBQ_packCustomCB__(FuncT customCB) noexcept
{
    CBQArg_t arg;
    arg.pVar = reinterpret_cast<void*>(reinterpret_cast<int (*)(ArgsT...)>(+customCB));
    return arg;
}

template <typename FuncT, typename... ArgsT>
inline int Queue::Push(FuncT func, ArgsT... arguments) noexcept
{
    return CBQ_Push(&this->cbq, CBQ_invokeCustomCB__<ArgsT...>, 0, CBQ_NO_VPARAMS, sizeof...(arguments) + 1, CBQ_packCustomCB__<FuncT, ArgsT...>(func), CBQ_convertToArg__<ArgsT>(arguments)...);
}


template <typename... Args>
inline int Queue::SetTimeout(QCallback func, clock_t delay, Args... arguments) noexcept
{
    CBQArg_t params[] = {CBQ_convertToArg__<Args>(arguments)...};
    return CBQ_SetTimeout(&this->cbq, delay, 0, &this->cbq, func, sizeof...(arguments), sizeof...(arguments)? params : CBQ_NO_VPARAMS);
}

template <typename... Args>
inline int Queue::SetTimeout(Queue& target, QCallback func, clock_t delay, Args... arguments) noexcept
{
    CBQArg_t params[] = {CBQ_convertToArg__<Args>(arguments)...};
    return CBQ_SetTimeout(&this->cbq, delay, 0, &target.cbq, func, sizeof...(arguments), sizeof...(arguments)? params : CBQ_NO_VPARAMS);
}

template <typename... Args>
inline int Queue::SetTimeoutForSec(QCallback func, clock_t delayInSec, Args... arguments) noexcept
{
    CBQArg_t params[] = {CBQ_convertToArg__<Args>(arguments)...};
    return CBQ_SetTimeout(&this->cbq, delayInSec, 1, &this->cbq, func, sizeof...(arguments), sizeof...(arguments)? params : CBQ_NO_VPARAMS);
}

template <typename... Args>
inline int Queue::SetTimeoutForSec(Queue& target, QCallback func, clock_t delayInSec, Args... arguments) noexcept
{
    CBQArg_t* params = {CBQ_convertToArg__<Args>(arguments)...};
    return CBQ_SetTimeout(&this->cbq, delayInSec, 1, &target.cbq, func, sizeof...(arguments), sizeof...(arguments)? params : CBQ_NO_VPARAMS);
}

template <typename FuncT, typename... Args>
inline int Queue::SetTimeout(FuncT func, clock_t delay, Args... arguments) noexcept
{
    CBQArg_t params[] = { CBQ_packCustomCB__<FuncT, Args...>(func), CBQ_convertToArg__<Args>(arguments)... };
    return CBQ_SetTimeout(&this->cbq, delay, 0, &this->cbq, CBQ_invokeCustomCB__<Args...>, sizeof...(arguments) + 1, sizeof...(arguments)? params : CBQ_NO_VPARAMS);
}

template <typename FuncT, typename... Args>
inline int Queue::SetTimeout(Queue& target, FuncT func, clock_t delay, Args... arguments) noexcept
{
    CBQArg_t params[] = { CBQ_packCustomCB__<FuncT, Args...>(func), CBQ_convertToArg__<Args>(arguments)... };
    return CBQ_SetTimeout(&this->cbq, delay, 0, &target.cbq, CBQ_invokeCustomCB__<Args...>, sizeof...(arguments) + 1, sizeof...(arguments)? params : CBQ_NO_VPARAMS);
}

template <typename FuncT, typename... Args>
inline int Queue::SetTimeoutForSec(FuncT func, clock_t delayInSec, Args... arguments) noexcept
{
    CBQArg_t params[] = { CBQ_packCustomCB__<FuncT, Args...>(func), CBQ_convertToArg__<Args>(arguments)...};
    return CBQ_SetTimeout(&this->cbq, delayInSec, 1, &this->cbq, CBQ_invokeCustomCB__<Args...>, sizeof...(arguments) + 1, sizeof...(arguments)? params : CBQ_NO_VPARAMS);
}

template <typename FuncT, typename... Args>
inline int Queue::SetTimeoutForSec(Queue& target, FuncT func, clock_t delayInSec, Args... arguments) noexcept
{
    CBQArg_t* params = { CBQ_packCustomCB__<FuncT, Args...>(func), CBQ_convertToArg__<Args>(arguments)...};
    return CBQ_SetTimeout(&this->cbq, delayInSec, 1, &target.cbq, CBQ_invokeCustomCB__<Args...>, sizeof...(arguments) + 1, sizeof...(arguments)? params : CBQ_NO_VPARAMS);
}


inline int Queue::Execute(int* cb_status) noexcept
{
    return CBQ_Exec(&this->cbq, cb_status);
}

inline size_t Queue::Size(void) const noexcept
{
    size_t size;
    CBQ_GetSize(&this->cbq, &size);
    return size;
}

inline size_t Queue::Capacity(void) const noexcept
{
    return CBQ_GETCAPACITY(this->cbq);
}

inline size_t Queue::CapacityInBytes(void) const noexcept
{
    size_t cap;
    CBQ_GetCapacityInBytes(&this->cbq, &cap);
    return cap;
}

inline bool Queue::IsEmpty(void) const noexcept
{
    return static_cast<bool>(CBQ_ISEMPTY(this->cbq));
}

inline bool Queue::IsFull(void) const noexcept
{
    return static_cast<bool>(CBQ_ISFULL(this->cbq));
}

inline void Queue::DetailInfo
(size_t* size, size_t* capacity, int* incCapMode, size_t* maxCapLimit, size_t* sizeInBytes) const noexcept
{
    CBQ_GetDetailedInfo(&this->cbq, capacity, size, incCapMode, maxCapLimit, sizeInBytes);
}


inline int Queue::IncreaseCapacity(void) noexcept
{
    return CBQ_ChangeCapacity(&this->cbq, CBQ_INC_CAPACITY, 0, 1);
}

inline int Queue::DecreaseCapacity(void) noexcept
{
    return CBQ_ChangeCapacity(&this->cbq, CBQ_DEC_CAPACITY, 0, 1);
}

inline int Queue::ChangeCapacity(bool increase) noexcept
{
    return CBQ_ChangeCapacity(&this->cbq, increase? CBQ_INC_CAPACITY : CBQ_DEC_CAPACITY, 0, 1);
}

inline int Queue::ChangeCapacity(size_t newSize, bool considerLimits) noexcept
{
    return CBQ_ChangeCapacity(&this->cbq, CBQ_CUSTOM_CAPACITY, newSize, static_cast<int>(considerLimits));
}

inline int Queue::ChangeIncreaseCapacityMode(CBQ_CapacityModes newMode, size_t newLimit, bool adaptLimit, bool tryToAdaptCapacity) noexcept
{
    return CBQ_ChangeIncCapacityMode(&this->cbq, newMode, newLimit, static_cast<int>(tryToAdaptCapacity), static_cast<int>(adaptLimit));
}

inline int Queue::EqualizeArgumentsCapacity(unsigned int requiredCapacity, bool skipNoneModifiable) noexcept
{
    return CBQ_EqualizeArgsCapByCustom(&this->cbq, requiredCapacity, static_cast<int>(skipNoneModifiable));
}

inline int Queue::ChangeInitArgumentsCapacity(unsigned int newCapacity) noexcept
{
    return CBQ_ChangeInitArgsCapByCustom(&this->cbq, newCapacity);
}


inline int Queue::GetVerIndex(void)
{
    return CBQ_GetVerIndex();
}

inline bool Queue::CheckVerIndexByFlag(CBQ_VI fInfoType)
{
    return static_cast<bool>(CBQ_CheckVerIndexByFlag(fInfoType));
}

inline int Queue::GetDifferencesVerIndexMask(int comparedVerId)
{
    return CBQ_GetDifferencesVerIdMask(comparedVerId);
}

inline int Queue::GetAvaliableFlagsRange(void)
{
    return CBQ_GetAvaliableFlagsRange();
}

inline bool Queue::IsCustomisedVersion(void)
{
    return CBQ_IsCustomisedVersion();
}

}   // CBQPP namespace
