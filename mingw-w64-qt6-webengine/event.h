/**
 * wrl/event.h - Microsoft::WRL Callback / EventSource for mingw-w64.
 *
 * Port of the Windows SDK <wrl/event.h> to mingw-w64 (clang/gcc, Itanium
 * C++ ABI, no MS extensions). Delegate creation (Callback), weak reference
 * callbacks, EventSource / AgileEventSource and DeferrableEventArgs follow the
 * SDK implementation.
 *
 * Differences from the SDK header:
 *  - Wrappers::SRWLock is defined here because the mingw-w64
 *    <wrl/wrappers/corewrappers.h> lacks it.
 *  - EventSource invokes every delegate with copies of the InvokeAll arguments
 *    instead of forwarding (moving) the same arguments several times.
 *  - RoTransformError / RoOriginateError are resolved from combase.dll at
 *    runtime (see <wrl/implements.h>).
 */

#ifndef _WRL_EVENT_H_
#define _WRL_EVENT_H_

#include <windows.h>
#include <winerror.h>
#include <eventtoken.h>

#include <new>
#include <type_traits>
#include <utility>

#include <wrl/internal.h>
#include <wrl/client.h>
#include <wrl/implements.h>
#include <wrl/wrappers/corewrappers.h>

// WinRT HRESULTs missing from mingw-w64 <winerror.h>
#ifndef E_ILLEGAL_STATE_CHANGE
#define E_ILLEGAL_STATE_CHANGE _HRESULT_TYPEDEF_(0x8000000DL)
#endif
#ifndef E_ILLEGAL_METHOD_CALL
#define E_ILLEGAL_METHOD_CALL _HRESULT_TYPEDEF_(0x8000000EL)
#endif
#ifndef E_ILLEGAL_DELEGATE_ASSIGNMENT
#define E_ILLEGAL_DELEGATE_ASSIGNMENT _HRESULT_TYPEDEF_(0x80000018L)
#endif
#ifndef E_ASYNC_OPERATION_NOT_STARTED
#define E_ASYNC_OPERATION_NOT_STARTED _HRESULT_TYPEDEF_(0x80000019L)
#endif
#ifndef JSCRIPT_E_CANTEXECUTE
#define JSCRIPT_E_CANTEXECUTE _HRESULT_TYPEDEF_(0x89020001L)
#endif

#ifndef WrlFinal
#define WrlFinal final
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winaccessible-base"
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Winaccessible-base"
#pragma GCC diagnostic ignored "-Wsubobject-linkage"
#endif

#pragma region SRWLock wrapper
// Part of <wrl/wrappers/corewrappers.h> in the Windows SDK.
#ifndef __WRL_WRAPPERS_SRWLOCK_DEFINED__
#define __WRL_WRAPPERS_SRWLOCK_DEFINED__

namespace Microsoft {
namespace WRL {
namespace Wrappers {

class SRWLock;

namespace HandleTraits {

struct SRWLockSharedTraits
{
    typedef SRWLOCK* Type;

    inline static Type GetInvalidValue() noexcept
    {
        return nullptr;
    }

    inline static void Unlock(Type srwlock) noexcept
    {
        ::ReleaseSRWLockShared(srwlock);
    }
};

struct SRWLockExclusiveTraits
{
    typedef SRWLOCK* Type;

    inline static Type GetInvalidValue() noexcept
    {
        return nullptr;
    }

    inline static void Unlock(Type srwlock) noexcept
    {
        ::ReleaseSRWLockExclusive(srwlock);
    }
};

} // namespace HandleTraits

namespace Details {

// Exclusive lock object for SRWLock
class SyncLockExclusive
{
public:
    SyncLockExclusive(SyncLockExclusive&& other) noexcept : sync_(other.sync_)
    {
        other.sync_ = HandleTraits::SRWLockExclusiveTraits::GetInvalidValue();
    }

    SyncLockExclusive(const SyncLockExclusive&) = delete;
    SyncLockExclusive& operator=(const SyncLockExclusive&) = delete;

    ~SyncLockExclusive() noexcept
    {
        InternalUnlock();
    }

    void Unlock() noexcept
    {
        __WRL_ASSERT__(IsLocked());
        InternalUnlock();
    }

    bool IsLocked() const noexcept
    {
        return (sync_ != HandleTraits::SRWLockExclusiveTraits::GetInvalidValue());
    }

    friend class Wrappers::SRWLock;

protected:
    explicit SyncLockExclusive(SRWLOCK* sync = HandleTraits::SRWLockExclusiveTraits::GetInvalidValue()) noexcept : sync_(sync)
    {
    }

    SRWLOCK* sync_;

private:
    void InternalUnlock() noexcept
    {
        if (IsLocked())
        {
            HandleTraits::SRWLockExclusiveTraits::Unlock(sync_);
            sync_ = HandleTraits::SRWLockExclusiveTraits::GetInvalidValue();
        }
    }
};

// Shared lock object for SRWLock
class SyncLockShared
{
public:
    SyncLockShared(SyncLockShared&& other) noexcept : sync_(other.sync_)
    {
        other.sync_ = HandleTraits::SRWLockSharedTraits::GetInvalidValue();
    }

    SyncLockShared(const SyncLockShared&) = delete;
    SyncLockShared& operator=(const SyncLockShared&) = delete;

    ~SyncLockShared() noexcept
    {
        InternalUnlock();
    }

    void Unlock() noexcept
    {
        __WRL_ASSERT__(IsLocked());
        InternalUnlock();
    }

    bool IsLocked() const noexcept
    {
        return (sync_ != HandleTraits::SRWLockSharedTraits::GetInvalidValue());
    }

    friend class Wrappers::SRWLock;

protected:
    explicit SyncLockShared(SRWLOCK* sync = HandleTraits::SRWLockSharedTraits::GetInvalidValue()) noexcept : sync_(sync)
    {
    }

    SRWLOCK* sync_;

private:
    void InternalUnlock() noexcept
    {
        if (IsLocked())
        {
            HandleTraits::SRWLockSharedTraits::Unlock(sync_);
            sync_ = HandleTraits::SRWLockSharedTraits::GetInvalidValue();
        }
    }
};

} // namespace Details

// SRW lock implementation
class SRWLock
{
public:
    typedef Details::SyncLockExclusive SyncLockExclusive;
    typedef Details::SyncLockShared SyncLockShared;

    SRWLock() noexcept
    {
        ::InitializeSRWLock(&SRWLock_);
    }

    SRWLock(const SRWLock&) = delete;
    SRWLock& operator=(const SRWLock&) = delete;

    ~SRWLock() noexcept
    {
    }

    SyncLockExclusive LockExclusive() noexcept
    {
        return LockExclusive(&SRWLock_);
    }

    static SyncLockExclusive LockExclusive(SRWLOCK* lock) noexcept
    {
        ::AcquireSRWLockExclusive(lock);
        return SyncLockExclusive(lock);
    }

    SyncLockExclusive TryLockExclusive() noexcept
    {
        return TryLockExclusive(&SRWLock_);
    }

    static SyncLockExclusive TryLockExclusive(SRWLOCK* lock) noexcept
    {
        bool acquired = !!::TryAcquireSRWLockExclusive(lock);
        return SyncLockExclusive((acquired) ? lock : nullptr);
    }

    SyncLockShared LockShared() noexcept
    {
        return LockShared(&SRWLock_);
    }

    static SyncLockShared LockShared(SRWLOCK* lock) noexcept
    {
        ::AcquireSRWLockShared(lock);
        return SyncLockShared(lock);
    }

    SyncLockShared TryLockShared() noexcept
    {
        return TryLockShared(&SRWLock_);
    }

    static SyncLockShared TryLockShared(SRWLOCK* lock) noexcept
    {
        bool acquired = !!::TryAcquireSRWLockShared(lock);
        return SyncLockShared((acquired) ? lock : nullptr);
    }

protected:
    SRWLOCK SRWLock_;
};

} // namespace Wrappers
} // namespace WRL
} // namespace Microsoft

#endif // __WRL_WRAPPERS_SRWLOCK_DEFINED__
#pragma endregion // SRWLock wrapper

namespace Microsoft {
namespace WRL {

// Enum to specify the behavior on checking delegate return results
enum DelegateCheckMode
{
    NoCheck = 1
};

template <DelegateCheckMode delegateCheckMode> struct DelegateTraits;

template <>
struct DelegateTraits<NoCheck>
{
    static HRESULT CheckReturn(HRESULT hr)
    {
        return hr;
    }
};

inline constexpr DelegateCheckMode DefaultDelegateCheckMode = NoCheck;

// Enum to specify the behavior when firing event delegates
enum InvokeMode
{
   StopOnFirstError = 1,
   FireAll = 2,
};

// Event error options
template <InvokeMode invokeModeValue>
struct InvokeModeOptions
{
    static const InvokeMode invokeMode = invokeModeValue;
};

// Forward Declaration
template <InvokeMode invokeMode> struct InvokeTraits;
template <typename TDelegateInterface, typename EventSourceOptions>
class EventSource;

namespace Details
{

template <typename TDelegateInterface>
void* GetDelegateBucketAssist(TDelegateInterface* pDelegate)
{
    // By ABI contract, delegates satisfy the below as a mechanism of getting at the invocation
    // function (the fourth slot in the V-Table of the delegate interface).  We do not care about
    // the signature of the function, only its address as a means of improving bucketing.
    void*** pVT = reinterpret_cast<void***>(pDelegate);
    return (*pVT)[3];
}

// ArgTraits allows to determine amount of parameters
// on Invoke method of delegate interface
template <typename TMemberFunction>
struct ArgTraits
{
    static const int args = -1; // Indicates that we cannot match Invoke method signature
};

template <typename TDelegateInterface>
struct ArgTraits<HRESULT (STDMETHODCALLTYPE TDelegateInterface::*)(void)>
{
    static const int args = 0;
};

template <typename TDelegateInterface, typename TArg1>
struct ArgTraits<HRESULT (STDMETHODCALLTYPE TDelegateInterface::*)(TArg1)>
{
    static const int args = 1;
    typedef TArg1 Arg1Type;
};

template <typename TDelegateInterface, typename TArg1, typename TArg2>
struct ArgTraits<HRESULT (STDMETHODCALLTYPE TDelegateInterface::*)(TArg1, TArg2)>
{
    static const int args = 2;
    typedef TArg1 Arg1Type;
    typedef TArg2 Arg2Type;
};

template <typename TDelegateInterface, typename TArg1, typename TArg2, typename TArg3>
struct ArgTraits<HRESULT (STDMETHODCALLTYPE TDelegateInterface::*)(TArg1, TArg2, TArg3)>
{
    static const int args = 3;
    typedef TArg1 Arg1Type;
    typedef TArg2 Arg2Type;
    typedef TArg3 Arg3Type;
};

template <typename TDelegateInterface, typename TArg1, typename TArg2, typename TArg3, typename TArg4>
struct ArgTraits<HRESULT (STDMETHODCALLTYPE TDelegateInterface::*)(TArg1, TArg2, TArg3, TArg4)>
{
    static const int args = 4;
    typedef TArg1 Arg1Type;
    typedef TArg2 Arg2Type;
    typedef TArg3 Arg3Type;
    typedef TArg4 Arg4Type;
};

template <typename TDelegateInterface, typename TArg1, typename TArg2, typename TArg3, typename TArg4, typename TArg5>
struct ArgTraits<HRESULT (STDMETHODCALLTYPE TDelegateInterface::*)(TArg1, TArg2, TArg3, TArg4, TArg5)>
{
    static const int args = 5;
    typedef TArg1 Arg1Type;
    typedef TArg2 Arg2Type;
    typedef TArg3 Arg3Type;
    typedef TArg4 Arg4Type;
    typedef TArg5 Arg5Type;
};

template <typename TDelegateInterface, typename TArg1, typename TArg2, typename TArg3, typename TArg4, typename TArg5, typename TArg6>
struct ArgTraits<HRESULT (STDMETHODCALLTYPE TDelegateInterface::*)(TArg1, TArg2, TArg3, TArg4, TArg5, TArg6)>
{
    static const int args = 6;
    typedef TArg1 Arg1Type;
    typedef TArg2 Arg2Type;
    typedef TArg3 Arg3Type;
    typedef TArg4 Arg4Type;
    typedef TArg5 Arg5Type;
    typedef TArg6 Arg6Type;
};

template <typename TDelegateInterface, typename TArg1, typename TArg2, typename TArg3, typename TArg4, typename TArg5, typename TArg6, typename TArg7>
struct ArgTraits<HRESULT (STDMETHODCALLTYPE TDelegateInterface::*)(TArg1, TArg2, TArg3, TArg4, TArg5, TArg6, TArg7)>
{
    static const int args = 7;
    typedef TArg1 Arg1Type;
    typedef TArg2 Arg2Type;
    typedef TArg3 Arg3Type;
    typedef TArg4 Arg4Type;
    typedef TArg5 Arg5Type;
    typedef TArg6 Arg6Type;
    typedef TArg7 Arg7Type;
};

template <typename TDelegateInterface, typename TArg1, typename TArg2, typename TArg3, typename TArg4, typename TArg5, typename TArg6, typename TArg7, typename TArg8>
struct ArgTraits<HRESULT (STDMETHODCALLTYPE TDelegateInterface::*)(TArg1, TArg2, TArg3, TArg4, TArg5, TArg6, TArg7, TArg8)>
{
    static const int args = 8;
    typedef TArg1 Arg1Type;
    typedef TArg2 Arg2Type;
    typedef TArg3 Arg3Type;
    typedef TArg4 Arg4Type;
    typedef TArg5 Arg5Type;
    typedef TArg6 Arg6Type;
    typedef TArg7 Arg7Type;
    typedef TArg8 Arg8Type;
};

template <typename TDelegateInterface, typename TArg1, typename TArg2, typename TArg3, typename TArg4, typename TArg5, typename TArg6, typename TArg7, typename TArg8, typename TArg9>
struct ArgTraits<HRESULT (STDMETHODCALLTYPE TDelegateInterface::*)(TArg1, TArg2, TArg3, TArg4, TArg5, TArg6, TArg7, TArg8, TArg9)>
{
    static const int args = 9;
    typedef TArg1 Arg1Type;
    typedef TArg2 Arg2Type;
    typedef TArg3 Arg3Type;
    typedef TArg4 Arg4Type;
    typedef TArg5 Arg5Type;
    typedef TArg6 Arg6Type;
    typedef TArg7 Arg7Type;
    typedef TArg8 Arg8Type;
    typedef TArg9 Arg9Type;
};

// noexcept is part of the function type since C++17
template <typename TDelegateInterface, typename ...TArgs>
struct ArgTraits<HRESULT (STDMETHODCALLTYPE TDelegateInterface::*)(TArgs...) noexcept> :
    ArgTraits<HRESULT (STDMETHODCALLTYPE TDelegateInterface::*)(TArgs...)>
{
};

// Traits factory extract appropriate ArgTraits type
// for delegate interface
template <typename TDelegateInterface, bool isImplements = std::is_base_of<ImplementsBase, TDelegateInterface>::value>
struct ArgTraitsHelper;

// Specialization for the none Implements based interface
template <typename TDelegateInterface>
struct ArgTraitsHelper<TDelegateInterface, false>
{
    typedef decltype(&TDelegateInterface::Invoke) methodType;
    typedef ArgTraits<methodType> Traits;
    static const int args = Traits::args;
    typedef TDelegateInterface Interface;
    // Make sure that you are using STDMETHOD macro to define delegate interfaces
    static_assert(args != -1, "Delegate Invoke signature doesn't match. Possible reasons: wrong calling convention, wrong returned type or passed too many parameters to 'Callback'");
};

// Specialization for Implements based interface
template <typename TDelegateInterface>
struct ArgTraitsHelper<TDelegateInterface, true> : ArgTraitsHelper<typename TDelegateInterface::FirstInterface>
{
};

// Used to produce a delegate with the Invoke() method signature that matches that of TDelegateInterface.
template <typename TDelegateInterface> class DelegateArgTraits; // to be specialized

template <typename TDelegateInterface, typename ...TArgs>
class DelegateArgTraits<HRESULT (STDMETHODCALLTYPE TDelegateInterface::*)(TArgs...)>
{
    template <typename TOtherDelegateInterface, typename TCallback, DelegateCheckMode checkMode, typename... TOtherArgs>
    struct DelegateInvokeHelper WrlFinal :
        public ::Microsoft::WRL::RuntimeClass<RuntimeClassFlags<Delegate>, TOtherDelegateInterface>,
        std::remove_reference<TCallback>::type
    {
        DelegateInvokeHelper(TCallback&& callback) noexcept :
            std::remove_reference<TCallback>::type(std::forward<TCallback>(callback)) {}

        HRESULT STDMETHODCALLTYPE Invoke(TOtherArgs... args) noexcept override
        {
            return DelegateTraits<checkMode>::CheckReturn((*this)(std::forward<TOtherArgs>(args)...));
        }
    };

public:
    template <typename TOtherDelegateInterface, typename TImplements, DelegateCheckMode checkMode = DefaultDelegateCheckMode, typename TLambda>
    static ComPtr<TImplements> Callback(TLambda&& callback) noexcept
    {
        static_assert(__is_base_of(IUnknown, TOtherDelegateInterface) && !__is_base_of(IInspectable, TOtherDelegateInterface), "Delegates objects must be 'IUnknown' base and not 'IInspectable'");
        return Make<DelegateInvokeHelper<TOtherDelegateInterface, TLambda, checkMode, TArgs...>>(std::forward<TLambda>(callback));
    }
};

// noexcept is part of the function type since C++17
template <typename TDelegateInterface, typename ...TArgs>
class DelegateArgTraits<HRESULT (STDMETHODCALLTYPE TDelegateInterface::*)(TArgs...) noexcept> :
    public DelegateArgTraits<HRESULT (STDMETHODCALLTYPE TDelegateInterface::*)(TArgs...)>
{
};

// Traits factory extract appropriate ArgTraits type
// for delegate interface
template <typename TDelegateInterface, bool isImplements = std::is_base_of<ImplementsBase, TDelegateInterface>::value>
struct DelegateArgTraitsHelper;

// Specialization for the none Implements based interface
template <typename TDelegateInterface>
struct DelegateArgTraitsHelper<TDelegateInterface, false>
{
    typedef TDelegateInterface Interface;
    typedef DelegateArgTraits<decltype(&TDelegateInterface::Invoke)> Traits;
};

// Specialization for ImplementsBase interface, selects the first interface
template <typename TDelegateInterface>
struct DelegateArgTraitsHelper<TDelegateInterface, true> : DelegateArgTraitsHelper<typename TDelegateInterface::FirstInterface>
{
};

#ifdef __WRL_AGILEREF_DEFINED__  // Windows 8.1+ (see AgileRef in <wrl/implements.h>)
// If the input delegate is already agile this could return it directly instead of creating a wrapper.
template <typename TDelegateInterface>
HRESULT CreateAgileHelper(TDelegateInterface* delegateInterface, TDelegateInterface** wrapper)
{
    *wrapper = nullptr;
    static_assert(__is_base_of(IUnknown, TDelegateInterface) && !__is_base_of(IInspectable, TDelegateInterface), "Delegates objects must be 'IUnknown' base and not 'IInspectable'");

    AgileRef delegateAsAgile;
    HRESULT hr = Microsoft::WRL::AsAgile(delegateInterface, &delegateAsAgile);
    if (FAILED(hr)) return hr;

    using DelegateHelper = DelegateArgTraitsHelper<TDelegateInterface>;

    auto callback = DelegateHelper::Traits::template Callback<Implements<RuntimeClassFlags<ClassicCom>, TDelegateInterface, FtmBase>, typename DelegateHelper::Interface>(
        [delegateAsAgile = std::move(delegateAsAgile)](auto&&... args)
    {
        ComPtr<TDelegateInterface> localDelegate;
        HRESULT hr = delegateAsAgile.CopyTo(localDelegate.GetAddressOf());
        if (FAILED(hr)) return hr;
        return localDelegate->Invoke(std::forward<decltype(args)>(args)...);
    });

    if (!callback) return E_OUTOFMEMORY;
    *wrapper = callback.Detach();
    return S_OK;
}
#endif // __WRL_AGILEREF_DEFINED__

} // namespace Details


// Construct a COM/WinRT delegate (an object with an Invoke() method) from a lambda.
// Check the return from this function for null to detect out of memory (E_OUTOFMEMORY) failure case.
template <typename TDelegateInterface, typename TLambda>
ComPtr<typename Details::DelegateArgTraitsHelper<TDelegateInterface>::Interface> Callback(TLambda&& callback) noexcept
{
    using DelegateHelper = Details::DelegateArgTraitsHelper<TDelegateInterface>;
    return DelegateHelper::Traits::template Callback<TDelegateInterface, typename DelegateHelper::Interface>(std::forward<TLambda>(callback));
}

// Construct a COM/WinRT delegate, an object with an Invoke() method, from a raw function.
template <typename TDelegateInterface, typename TFunc>
ComPtr<typename Details::DelegateArgTraitsHelper<TDelegateInterface>::Interface> Callback(TFunc* callback) noexcept
{
    using DelegateHelper = Details::DelegateArgTraitsHelper<TDelegateInterface>;
    return DelegateHelper::Traits::template Callback<TDelegateInterface, typename DelegateHelper::Interface>(
        [=](auto&& ...args)
    {
        return callback(std::forward<decltype(args)>(args)...);
    });
}

// Construct a COM/WinRT delegate, an object with an Invoke() method, from an object and member function.
template <typename TDelegateInterface, typename TCallbackObject, typename... TArgs>
ComPtr<typename Details::DelegateArgTraitsHelper<TDelegateInterface>::Interface> Callback(TCallbackObject* object, HRESULT (TCallbackObject::* method)(TArgs...)) noexcept
{
    return Callback<TDelegateInterface>([=](auto&& ...args) { return ((*object).*(method))(args ...); });
}

// Provide a callback that holds a weak reference to an innner callback to invoke. This is useful when breaking cycles.
template <typename TDelegateInterface>
HRESULT WeakReferenceCallback(IWeakReferenceSource* innerCallback, TDelegateInterface** callback)
{
    *callback = nullptr;
    WeakRef innerWeakRef;
    HRESULT hr = innerCallback->GetWeakReference(&innerWeakRef);
    if (SUCCEEDED(hr))
    {
        *callback = Callback<TDelegateInterface>([innerWeakRef = std::move(innerWeakRef)](auto&&... args) -> HRESULT
        {
            ComPtr<TDelegateInterface> strongHandler;
            // Attempt to resolve directly to the delegate. Note that the delegate likely doesn't derive from IInspectable. Nonetheless,
            // this shortcut is generally considered safe. By default, WeakRef::As() blocks this since this is an unusual case.
            HRESULT hr = innerWeakRef.Get()->Resolve(__uuidof(TDelegateInterface), reinterpret_cast<IInspectable**>(strongHandler.GetAddressOf()));
            if (SUCCEEDED(hr))
            {
                if (strongHandler)
                {
                    return strongHandler->Invoke(args ...);
                }
                else
                {
                    // This will signal to an event or async completion that the server isn't available. It'll be ignored as a failure,
                    // but in events, will cause the event source to drop the callback, shorting out the need to resolve the weak reference again later.
                    return RPC_E_DISCONNECTED;
                }
            }
            return hr;
        }).Detach();
        hr = (*callback != nullptr) ? S_OK : E_OUTOFMEMORY;
    }
    return hr;
}

template <typename TDelegateInterface>
HRESULT WeakReferenceCallback(TDelegateInterface* innerCallback, TDelegateInterface** callback)
{
    *callback = nullptr;
    ComPtr<IWeakReferenceSource> weakRefSource;
    HRESULT hr = innerCallback->QueryInterface(__uuidof(IWeakReferenceSource), reinterpret_cast<void**>(weakRefSource.GetAddressOf()));
    if (SUCCEEDED(hr))
    {
        hr = WeakReferenceCallback(weakRefSource.Get(), callback);
    }
    return hr;
}

// Make a callback to a specific method on a class through a weak pointer. The class must implement IWeakReferenceSource and the method signature must
// match the delegate Invoke signature, but the class does not need to implement the specific delegate interface, and the member method does not need to be called "Invoke".
template <typename T, typename TDelegateInterface, typename ...TArgs>
HRESULT WeakReferenceCallback(T* targetObject, HRESULT (T::*targetMethod)(TArgs... args), TDelegateInterface** callback)
{
    static_assert(__is_base_of(IWeakReferenceSource, T), "The object being called must be able to supply a weak reference to be called back through.");

    *callback = nullptr;
    WeakRef innerWeakRef;
    HRESULT hr = targetObject->GetWeakReference(&innerWeakRef);
    if (SUCCEEDED(hr))
    {
        *callback = Callback<TDelegateInterface>([innerWeakRef = std::move(innerWeakRef), targetObject, targetMethod](TArgs&&... innerArgs)
        {
            // Attempt to resolve directly to the delegate. Note that the delegate likely doesn't derive from IInspectable. Nonetheless,
            // this shortcut is generally considered safe. By default, WeakRef::As() blocks this since this is an unusual case.
            ComPtr<IInspectable> strongHandler;
            HRESULT hr = innerWeakRef.Get()->Resolve(__uuidof(IInspectable), reinterpret_cast<IInspectable**>(strongHandler.GetAddressOf()));
            if (SUCCEEDED(hr))
            {
                // Assumption is that targetObject is still valid if the inner weak reference resolved. There are some corner cases
                // that can break this (e.g. tear-offs) but this safe in normal cases. WeakRef resolves to IInspectable, which T may multiply-inherit
                // from, so it's frequently not possible to static-cast from that pointer to T.
                if (strongHandler)
                {
                    return (targetObject->*targetMethod)(std::forward<TArgs>(innerArgs) ...);
                }
                else
                {
                    // This will signal to an event or async completion that the server isn't available. It'll be ignored as a failure,
                    // but in events, will cause the event source to drop the callback, shorting out the need to resolve the weak reference again later.
                    return RPC_E_DISCONNECTED;
                }
            }
            return hr;
        }).Detach();

        hr = (*callback != nullptr) ? S_OK : E_OUTOFMEMORY;
    }
    return hr;
}

// The explicit overload is required here to support template argument deduction of the callback interface type.
template <typename T, typename TDelegateInterface, typename ...TArgs>
HRESULT WeakReferenceCallback(T* targetObject, HRESULT (T::*targetMethod)(TArgs... args), ::Microsoft::WRL::Details::ComPtrRef< ::Microsoft::WRL::ComPtr<TDelegateInterface>> callback)
{
    return WeakReferenceCallback(targetObject, targetMethod, static_cast<TDelegateInterface**>(callback));
}

namespace Details
{

// EventTargetArray is used to keep array of event targets. This array is fixed-length.
// Every time element is added/removed from array EventSource allocate new array. This array
// is optimize-for-invoke lock strategy in EventSource
class EventTargetArray WrlFinal : public ::Microsoft::WRL::RuntimeClass< ::Microsoft::WRL::RuntimeClassFlags<ClassicCom>, IUnknown>
{
    public:
        EventTargetArray() noexcept : begin_(nullptr), end_(nullptr), bucketAssists_(nullptr)
        {
        }

        HRESULT RuntimeClassInitialize(size_t items) noexcept
        {
            begin_ = new (std::nothrow) ComPtr<IUnknown>[items];
            bucketAssists_ = new (std::nothrow) void*[items];
            if (begin_ == nullptr || bucketAssists_ == nullptr)
            {
                // Don't check against nullptr because delete does it
                delete[] begin_;
                delete[] bucketAssists_;

                // Set member pointers to nullptr so destructor does not try to delete them again
                begin_ = nullptr;
                bucketAssists_ = nullptr;

                return E_OUTOFMEMORY;
            }

            end_ = begin_;
            return S_OK;
        }

        ~EventTargetArray() noexcept
        {
            // Don't check against nullptr because delete does it
            delete[] begin_;
            delete[] bucketAssists_;
        }

        ComPtr<IUnknown>* Begin() noexcept
        {
            return begin_;
        }

        ComPtr<IUnknown>* End() noexcept
        {
            return end_;
        }

        void AddTail(IUnknown* element) noexcept
        {
            AddTail(element, nullptr);
        }

        void AddTail(IUnknown* element, void* bucketAssist) noexcept
        {
            // We'll run over the end if you call AddTail too many times, but the alternate would be an extra variable
            // to keep track of the number of items allocated. This class is only privately used by EventSourceBase.
            *end_ = element;
            *(bucketAssists_ + (end_ - begin_)) = bucketAssist;
            end_++;
        }

        size_t Length() noexcept
        {
            return static_cast<size_t>(end_ - begin_);
        }

        void** Begin_BucketAssists()
        {
            return bucketAssists_;
        }

        void** End_BucketAssists()
        {
            return bucketAssists_ + (end_ - begin_);
        }

    private:
        ComPtr<IUnknown>* begin_;
        ComPtr<IUnknown>* end_;
        void** bucketAssists_;
};

inline bool IsDisconnectedDelegateError(HRESULT hr) noexcept
{
    return hr == RPC_E_DISCONNECTED || hr == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE) || hr == JSCRIPT_E_CANTEXECUTE;
}

} // namespace Details

template <>
struct InvokeTraits<FireAll>
{
   template <typename TInvokeMethod, typename TDelegateInterface>
   static HRESULT InvokeDelegates(TInvokeMethod invokeOne, Details::EventTargetArray* targetArray, EventSource<TDelegateInterface, InvokeModeOptions<FireAll>>* pEvent)
   {
      ComPtr<Details::EventTargetArray> targets;
      targets = targetArray;

      for (auto element = targets->Begin(); element != targets->End(); element++)
      {
          HRESULT hr = (invokeOne)(*element);
          if (FAILED(hr))
          {
              Details::RoTransformError(hr, S_OK, nullptr);
              // Remove event that is already disconnected
              if (Details::IsDisconnectedDelegateError(hr))
              {
                  EventRegistrationToken token;
                  token.value = reinterpret_cast<INT64>(element->Get());
                  pEvent->Remove(token);
              }
          }
      }
      return S_OK;
   }
};

template <>
struct InvokeTraits<StopOnFirstError>
{
   template <typename TInvokeMethod, typename TDelegateInterface>
   static HRESULT InvokeDelegates(TInvokeMethod invokeOne, Details::EventTargetArray* targetArray, EventSource<TDelegateInterface, InvokeModeOptions<StopOnFirstError>>* pEvent)
   {
      HRESULT hr = S_OK;
      ComPtr<Details::EventTargetArray> targets;
      targets = targetArray;

      for (auto element = targets->Begin(); element != targets->End(); element++)
      {
           hr = (invokeOne)(*element);
           // Remove event that is already disconnected
           if (Details::IsDisconnectedDelegateError(hr))
           {
               // if we get the above errors, treat it as success and unregister the delegate
               Details::RoTransformError(hr, S_OK, nullptr);
               EventRegistrationToken token;
               token.value = reinterpret_cast<INT64>(element->Get());
               pEvent->Remove(token);
               hr = S_OK;
           }
           if (FAILED(hr))
           {
              // break out of the loop on the first unhandled error
              break;
           }
      }
      return hr;
   }
};

// Source events for non-agile single threaded components. Agile components should use AgileEventSource
template <typename TDelegateInterface, typename TEventSourceOptions = InvokeModeOptions<FireAll>>
class EventSource
{
public:
    EventSource() noexcept :
         targets_(nullptr)
    {
    }

    HRESULT Add(TDelegateInterface* delegateInterface, EventRegistrationToken* token) noexcept
    {
        // Make sure that delegate interface pointer is not null
        if (delegateInterface == nullptr)
        {
            return E_INVALIDARG;
        }

        return AddInternal(delegateInterface, Microsoft::WRL::Details::GetDelegateBucketAssist(delegateInterface), token);
    }

    HRESULT Remove(EventRegistrationToken token) noexcept
    {
        // Used for deleting the current array without holding the addRemoveLock.
        ComPtr<Details::EventTargetArray> pTempList;
        { // lock scope for addRemoveLock_
            // The addRemoveLock_ prevents multiple threads from doing simultaneous adds/removes.
            // An invoke may be occurring during an add or remove operation.
            Wrappers::SRWLock::SyncLockExclusive addRemoveLock = addRemoveLock_.LockExclusive();

            if (targets_ == nullptr)
            {
                return S_OK; // List is currently empty - thus token wasn't found, just return
            }

            ComPtr<Details::EventTargetArray> pNewList;
            size_t availableSlots = targets_->Length() - 1;
            bool removed = false;
            // If one element in the array
            if (availableSlots == 0)
            {
                if (reinterpret_cast<INT64>(targets_->Begin()->Get()) == token.value)
                {
                    removed = true;
                }
            }
            else
            {
                // Instantiate EventTargetArray
                HRESULT hr = MakeAndInitialize<Details::EventTargetArray>(pNewList.GetAddressOf(), availableSlots);
                if (FAILED(hr))
                {
                    return hr;
                }

                void** bucketElement = targets_->Begin_BucketAssists();

                for (auto element = targets_->Begin(); element != targets_->End(); element++, bucketElement++)
                {
                    if (!removed && token.value == reinterpret_cast<INT64>(element->Get()))
                    {
                        removed = true;
                        continue;
                    }

                    if (availableSlots == 0)
                    {
                        // We don't have any availableSlots left in the target array, hence every item was copied
                        // from the source array.
                        // This means we didn't find the item in the list - just return.
                        __WRL_ASSERT__(!removed && "Attempt to remove token that was not added to this EventSource<>");
                        break;
                    }

                    // Copy every registrant from old list except the item being removed
                    // The ComPtr<TDelegateInterface> contained in p is assigned to a
                    // ComPtr<TDelegateInterface> of a new node in pNewList. The net result is
                    // an addref on the interface.
                    pNewList->AddTail(element->Get(), *bucketElement);
                    availableSlots--;
                }
            }

            if (removed)
            {
                // lock scope for targetsPointerLock
                // The targetsPointerLock_ protects the exchanging of the new list (with the removal)
                // for the old list (which could be used currently for firing events)
                Wrappers::SRWLock::SyncLockExclusive targetsPointerLock = targetsPointerLock_.LockExclusive();

                // We move targets_ to pTempList so that we can actually delete the list while
                // not holding any locks. The InvokeAll method may still have a reference to targets_ so
                // even when pTempList releases, this might not delete what was in targets_.
                // (Swap is used because the mingw-w64 ComPtr move assignment copies.)
                pTempList.Swap(targets_);

                // We still have some items left inside pNewList, so move it to targets_.
                // If pNewList is empty, targets_ becomes nullptr.
                targets_.Swap(pNewList);

            } // end lock scope for targetsPointerLock

        } // end lock scope for addRemoveLock

        // Destroys pTempList here (this is the old targets_)

        return S_OK;
    }

protected:

    HRESULT Add(TDelegateInterface* delegateInterface, void* bucketAssist, EventRegistrationToken* token) noexcept
    {
        // Make sure that delegate interface pointer is not null
        if (delegateInterface == nullptr)
        {
            return E_INVALIDARG;
        }

        return AddInternal(delegateInterface, bucketAssist, token);
    }

private:

    HRESULT AddInternal(TDelegateInterface* delegateInterface, void* bucketAssist, EventRegistrationToken* token) noexcept
    {
        // Clear token value
        token->value = 0;

        // This must be defined outside of the scope where the addRemoveLock is taken to ensure that it's destructor is called after the lock is released
        ComPtr<Details::EventTargetArray> pTempList;

        { // lock scope for addRemoveLock
            // We are doing "copy to new list and add" so as not to disturb the list that may be
            // currently undergoing a walk and fire (invoke).

            // The addRemoveLock_ prevents multiple threads from doing simultaneous adds.
            // An invoke may be occurring during an add or remove operation.
            Wrappers::SRWLock::SyncLockExclusive addRemoveLock = addRemoveLock_.LockExclusive();

            ComPtr<Details::EventTargetArray> pNewList;

            // Allocate event array
            HRESULT hr = MakeAndInitialize<Details::EventTargetArray>(pNewList.GetAddressOf(), targets_ == nullptr ? 1 : targets_->Length() + 1);
            // Make sure allocation succeeded
            if (FAILED(hr))
            {
                return hr;
            }

            // The list may not exist if nobody has registered
            if (targets_)
            {
                void** bucketElement = targets_->Begin_BucketAssists();
                for (auto element = targets_->Begin(); element != targets_->End(); element++)
                {
                    // The ComPtr<TDelegateInterface> contained in the current targets_ node
                    // is assigned to a ComPtr<TDelegateInterface> of a new node in pNewList
                    // the net result is an addref on the interface.

                    pNewList->AddTail(element->Get(), *bucketElement);
                    bucketElement++;
                }
            }

            // Get unique token value
            token->value = reinterpret_cast<INT64>(delegateInterface);

            // AddTail operation will take a reference which will result in
            // this function adding one reference count on delegateInterface.
            pNewList->AddTail(delegateInterface, bucketAssist);

            {
                // lock scope for targetsPointerLock
                // The targetsPointerLock_ protects the exchanging of the new list (with the addition)
                // for the old list (which could be used currently for firing events)
                Wrappers::SRWLock::SyncLockExclusive targetsPointerLock = targetsPointerLock_.LockExclusive();

                // We move targets_ to pTempList so that we can actually delete the list while
                // not holding any locks. The InvokeAll method may still have a reference to targets_ so
                // even when pTempList releases, this might not delete what was in targets_.
                pTempList.Swap(targets_);

                // We're done with pNewList, so just move it to targets_.
                targets_.Swap(pNewList);

            } // end lock scope for targetsPointerLock
        } // end lock scope for addRemoveLock

        // Destroys pTempList here (this is the old targets_)

        return S_OK;
    }

    // TInvokeMethod is a functor that performs the appropriate invoke, depending on the
    // number of arguments specified.
    template <typename TInvokeMethod>
    HRESULT DoInvoke(TInvokeMethod invokeOne) noexcept
    {
        HRESULT hr = S_OK;
        // The targetsPointerLock_ protects the acquisition of an AddRef'd pointer to
        // "current list".  An Add/Remove operation may occur during the
        // firing of events (but occurs on a copy of the list).  i.e. both
        // InvokeAll/invoke and Add/Remove are readers of the "current list".
        // NOTE:  EventSource<TDelegateInterface>::InvokeAll(...) must never take the addRemoveLock_.
        ComPtr<Details::EventTargetArray> targets;

        // Scoping for lock
        {
            Wrappers::SRWLock::SyncLockExclusive targetsPointerLock = targetsPointerLock_.LockExclusive();
            targets = targets_;
        }

        // The list may not exist if nobody has registered
        if (targets)
        {
            hr = InvokeTraits<TEventSourceOptions::invokeMode>::InvokeDelegates(invokeOne, targets.Get(), this);
        }
        return hr;
    }

public:

    template <typename ...TArgs>
    HRESULT InvokeAll(TArgs... args) noexcept
    {
        // Every delegate receives the same (copied) arguments; forwarding would move from
        // the arguments once per delegate.
        return DoInvoke([&](ComPtr<IUnknown>& p) -> HRESULT { return static_cast<TDelegateInterface*>(p.Get())->Invoke(args...); });
    }

    size_t GetSize() const noexcept
    {
        if (targets_ == nullptr)
        {
            return 0;
        }

        Wrappers::SRWLock::SyncLockExclusive targetsPointerLock = targetsPointerLock_.LockExclusive();
        return targets_ == nullptr ? 0 : targets_->Length();
    }

protected:
    ComPtr<Details::EventTargetArray> targets_;
    mutable Wrappers::SRWLock targetsPointerLock_;
    Wrappers::SRWLock addRemoveLock_;
};

#ifdef __windows_foundation_h__

template <typename TEventArgsInterface, typename TEventArgsClass>
class DeferrableEventArgs : public TEventArgsInterface
{
public:
    STDMETHOD(GetDeferral)(::ABI::Windows::Foundation::IDeferral** result)
    {
        *result = nullptr;
        auto lockGuard = lock_.LockExclusive();
        if (raised_)
        {
            // Cannot ask for a deferral after the event handler returned.
            Details::RoOriginateError(E_ILLEGAL_METHOD_CALL, nullptr);
            return E_ILLEGAL_METHOD_CALL;
        }

        Microsoft::WRL::ComPtr< ::ABI::Windows::Foundation::IDeferralFactory> factory;
        HRESULT hr = ::ABI::Windows::Foundation::GetActivationFactory(
            Wrappers::HStringReference(RuntimeClass_Windows_Foundation_Deferral).Get(),
            &factory);
        if (FAILED(hr))
        {
            return hr;
        }

        Microsoft::WRL::ComPtr<DeferrableEventArgs> lifetime(this);
        auto callback = Microsoft::WRL::Callback< ::ABI::Windows::Foundation::IDeferralCompletedHandler>(
                [lifetime]() { return lifetime->Complete(); });
        if (callback == nullptr)
        {
            return E_OUTOFMEMORY;
        }

        Microsoft::WRL::ComPtr< ::ABI::Windows::Foundation::IDeferral> deferral;
        hr = factory->Create(callback.Get(), &deferral);
        if (FAILED(hr))
        {
            return hr;
        }

        completionsRequired_++;
        return deferral.CopyTo(result);
    }

    // InvokeAllFinished() should be called after the event source calls InvokeAll.  This will prevent further
    // deferrals from being taken, and cause the completion handler to execute if no deferrals were taken.
    void InvokeAllFinished()
    {
        bool invokeNeeded;

        // We need to hold a lock while modifying private state, but release it before invoking a completion handler.
        {
            auto lockGuard = lock_.LockExclusive();
            raised_ = true;
            invokeNeeded = (completionsRequired_ == 0);
        }

        if (invokeNeeded)
        {
            static_cast<TEventArgsClass*>(this)->InvokeCompleteHandler();
        }
    }

private:
    HRESULT Complete()
    {
        bool invokeNeeded;

        // We need to hold a lock while modifying private state, but release it before invoking a completion handler.
        {
            auto lockGuard = lock_.LockExclusive();
            if (completionsRequired_ == 0)
            {
                // This should never happen since Complete() should only be called by Windows.Foundation.Deferral
                // which will only invoke our completion handler once.
                Details::RoOriginateError(E_ILLEGAL_METHOD_CALL, nullptr);
                return E_ILLEGAL_METHOD_CALL;
            }
            completionsRequired_--;
            invokeNeeded = (raised_ && (completionsRequired_ == 0));
        }

        if (invokeNeeded)
        {
            static_cast<TEventArgsClass*>(this)->InvokeCompleteHandler();
        }

        return S_OK;
    }

    Wrappers::SRWLock lock_;
    bool raised_ = false;
    long completionsRequired_ = 0;
};

#endif // __windows_foundation_h__

#ifdef __WRL_AGILEREF_DEFINED__  // Windows 8.1+ (see AgileRef in <wrl/implements.h>)
// WinRT event source implementation for agile/multi-threaded components.
template <typename TDelegateInterface, typename TEventSourceOptions = Microsoft::WRL::InvokeModeOptions<FireAll>>
class AgileEventSource : public Microsoft::WRL::EventSource<TDelegateInterface, TEventSourceOptions>
{
    using Super = Microsoft::WRL::EventSource<TDelegateInterface, TEventSourceOptions>;
public:
    HRESULT Add(TDelegateInterface* delegateInterface, EventRegistrationToken* token)
    {
        if (delegateInterface == nullptr)
        {
            // We do not want to store a null interface pointer in the event list. This makes the behavior of
            // AgileEvent similar to the behavior of Event.
            return E_INVALIDARG;
        }

        Microsoft::WRL::ComPtr<TDelegateInterface> agileCallback;
        HRESULT hr = Details::CreateAgileHelper<TDelegateInterface>(delegateInterface, &agileCallback);
        if (SUCCEEDED(hr))
        {
            hr = Super::Add(agileCallback.Get(), Microsoft::WRL::Details::GetDelegateBucketAssist(delegateInterface), token);
        }
        return hr;
    }
};
#endif // __WRL_AGILEREF_DEFINED__

} } // namespace ::Microsoft::WRL

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif // _WRL_EVENT_H_
