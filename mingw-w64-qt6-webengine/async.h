/**
 * wrl/async.h - Microsoft::WRL::AsyncBase for mingw-w64.
 *
 * Port of the Windows SDK <wrl/async.h> to mingw-w64. The async state machine,
 * delegate handling and error propagation policies follow the SDK.
 *
 * Differences from the SDK header:
 *  - Causality tracing (Windows.Foundation.Diagnostics) is compiled out, since
 *    mingw-w64 has no windows.foundation.diagnostics.h. The option types
 *    (AsyncCausalityOptions, DisableCausality, ...) are still accepted.
 *  - mingw-w64 declares IAsyncInfo / AsyncStatus in the global namespace; they
 *    are also made visible as ABI::Windows::Foundation::IAsyncInfo/AsyncStatus
 *    (define __WRL_NO_ABI_ASYNCINFO_ALIAS__ to disable).
 *  - Restricted error info and RoOriginateError/RoTransformError are resolved
 *    from combase.dll at runtime.
 */

#ifndef _WRL_ASYNC_H_
#define _WRL_ASYNC_H_

#include <windows.h>
#include <winerror.h>
#include <asyncinfo.h>
#include <windows.foundation.h>

#include <wrl/internal.h>
#include <wrl/client.h>
#include <wrl/implements.h>
#include <wrl/event.h>  // ArgTraitsHelper, GetDelegateBucketAssist, WinRT HRESULTs

#ifndef __WRL_NO_ABI_ASYNCINFO_ALIAS__
namespace ABI {
namespace Windows {
namespace Foundation {
using ::IAsyncInfo;
using ::AsyncStatus;
}
}
}
#endif

namespace Microsoft {
namespace WRL {

// Designates the error propagation policy used by FireProgress and FireComplete. If PropagateDelegateError is the mode
// then failures returned from the async completion and progress delegates are propagated.  If IgnoreDelegateError
// is the mode, then failures returned from the async completion and progress delegates are converted to successes and
// the errors are swallowed.
enum ErrorPropagationPolicy
{
    PropagateDelegateError = 1,
    IgnoreDelegateError = 2
};

namespace Details
{
    // contains states indicating existance or lack of options
    struct AsyncOptionsBase
    {
        static const bool hasCausalityOptions = false;
        static const bool hasErrorPropagationPolicy = false;
        static const bool hasCausalityOperationName = false;
        static const bool isCausalityEnabled = true;
    };

    template <PCWSTR OpName>
    struct IsOperationName
    {
        static const bool Value = true;
    };

    template <>
    struct IsOperationName<nullptr>
    {
        static const bool Value = false;
    };

    // Stand-in for ::ABI::Windows::Foundation::Diagnostics::CausalitySource
    enum CausalitySource
    {
        CausalitySource_Application = 0,
        CausalitySource_Library = 1,
        CausalitySource_System = 2
    };

    // mingw-w64 does not declare the restricted error info API, resolve lazily.
    // The IRestrictedErrorInfo object is only passed through, so IUnknown is enough.
    inline HRESULT GetRestrictedErrorInfo(IUnknown** errorInfo) noexcept
    {
        typedef HRESULT (WINAPI *Fn)(IUnknown**);
        static const Fn fn = reinterpret_cast<Fn>(reinterpret_cast<void*>(GetCombaseProc("GetRestrictedErrorInfo")));
        *errorInfo = nullptr;
        return fn != nullptr ? fn(errorInfo) : E_NOTIMPL;
    }

    inline HRESULT SetRestrictedErrorInfo(IUnknown* errorInfo) noexcept
    {
        typedef HRESULT (WINAPI *Fn)(IUnknown*);
        static const Fn fn = reinterpret_cast<Fn>(reinterpret_cast<void*>(GetCombaseProc("SetRestrictedErrorInfo")));
        return fn != nullptr ? fn(errorInfo) : E_NOTIMPL;
    }
} // namespace Details

// Options for error propagation and the defaults are set here.
template <ErrorPropagationPolicy errorPropagationPolicy = Microsoft::WRL::IgnoreDelegateError>
struct ErrorPropagationOptions : public Microsoft::WRL::Details::AsyncOptionsBase
{
    static const ErrorPropagationPolicy PropagationPolicy = errorPropagationPolicy;
    static const bool hasErrorPropagationPolicy = true;
};

// Options for causality tracing. Accepted for source compatibility, tracing is
// not available on mingw-w64.
template <
    PCWSTR OpName = nullptr,
    const GUID& PlatformId = GUID_NULL,
    Details::CausalitySource CausalitySource = Details::CausalitySource_Application
>
struct AsyncCausalityOptions : public Microsoft::WRL::Details::AsyncOptionsBase
{
    static PCWSTR GetAsyncOperationName()
    {
        return OpName;
    }

    static const GUID GetPlatformId()
    {
        return PlatformId;
    }

    static Details::CausalitySource GetCausalitySource()
    {
        return CausalitySource;
    }

    static const bool hasCausalityOptions = true;
    static const bool hasCausalityOperationName = Microsoft::WRL::Details::IsOperationName<OpName>::Value;
};

// This option type for causality tracing disables just the tracing part.
inline constexpr WCHAR DisableCausalityAsyncOperationName[] = L"Disabled";
struct DisableCausality : public AsyncCausalityOptions<DisableCausalityAsyncOperationName>
{
    static const bool isCausalityEnabled = false;
};

namespace Details
{
// maps internal definitions for AsyncStatus and defines states that are not client visible
enum AsyncStatusInternal
{
    // non-client visible internal states
    _Undefined = -2,
    _Created = -1,

    // client visible states (must match AsyncStatus exactly)
    _Started = static_cast<int>(::AsyncStatus::Started),
    _Completed = static_cast<int>(::AsyncStatus::Completed),
    _Canceled = static_cast<int>(::AsyncStatus::Canceled),
    _Error = static_cast<int>(::AsyncStatus::Error),

    // non-client visible internal states
    _Closed
};

template <typename T>
struct DerefHelper;

template <typename T>
struct DerefHelper<T*>
{
    typedef T DerefType;
};


#pragma region AsyncOptionsHelper

// helper class to switch between default or given options for error
// propagation
template <bool hasValue, typename TOptions>
struct ErrorPropagationOptionsHelper;

// provides the given options for error propagation
template <typename TOptions>
struct ErrorPropagationOptionsHelper<true, TOptions>
{
    static const Microsoft::WRL::ErrorPropagationPolicy PropagationPolicy = TOptions::PropagationPolicy;
};

// provides default options for error propagation
template <typename TOptions>
struct ErrorPropagationOptionsHelper<false, TOptions>
{
    static const Microsoft::WRL::ErrorPropagationPolicy PropagationPolicy = Microsoft::WRL::ErrorPropagationOptions<>::PropagationPolicy;
};

// helper calls to accumulate all options
// Future options have to be added here
template <typename TComplete, typename TOptions>
struct AsyncOptionsHelper :
    public ErrorPropagationOptionsHelper<TOptions::hasErrorPropagationPolicy, TOptions>
{
    static const bool CausalityEnabled = false;
};

#pragma endregion
// End of AsyncOptionsHelper

} // namespace Details

// designates whether the "GetResults" method returns a single result (after complete fires) or multiple results
// (which are progressively consumable between Start state and before Close is called)
enum AsyncResultType
{
    SingleResult    = 0x0001,
    MultipleResults = 0x0002
};

// indicates how an attempt to transition to a terminal state of Completed or Error should behave with respect to
// the (client-requested) Canceled state.
enum CancelTransitionPolicy
{
    // If the async operation is presently in a (client-requested) Canceled state, this indicates that
    // it will stay in the Canceled state as opposed to transitioning to a terminal Completed or Error
    // state.
    RemainCanceled = 0,

    // If the async operation is presently in a (client-requested) Canceled state, this indicates that
    // state should transition from that Canceled state to the terminal state of Completed or Error as
    // determined by the call utilizing this flag.
    TransitionFromCanceled
};

#pragma region AsyncOptions

template <ErrorPropagationPolicy errorPropagationPolicy>
struct ErrorPropagationPolicyTraits;

// This error propagation policy passes through all errors
template <>
struct ErrorPropagationPolicyTraits<PropagateDelegateError>
{
    static HRESULT FireCompletionErrorPropagationPolicyFilter(HRESULT hrIn, IUnknown*, void* = nullptr)
    {
        // Ignore errors if the error is caused by a disconnected object
        if (hrIn == RPC_E_DISCONNECTED || hrIn == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE) || hrIn == JSCRIPT_E_CANTEXECUTE)
        {
            Details::RoTransformError(hrIn, S_OK, nullptr);
            hrIn = S_OK;
        }
        return hrIn;
    }

    static HRESULT FireProgressErrorPropagationPolicyFilter(HRESULT hrIn, IUnknown*, void* = nullptr)
    {
        // Ignore errors if the error is caused by a disconnected object
        if (hrIn == RPC_E_DISCONNECTED || hrIn == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE) || hrIn == JSCRIPT_E_CANTEXECUTE)
        {
            Details::RoTransformError(hrIn, S_OK, nullptr);
            hrIn = S_OK;
        }
        return hrIn;
    }
};

// This error propagation policy ignores all errors and converts them to S_OK
template <>
struct ErrorPropagationPolicyTraits<IgnoreDelegateError>
{
    static HRESULT FireCompletionErrorPropagationPolicyFilter(HRESULT hrIn, IUnknown*, void* = nullptr)
    {
        if (FAILED(hrIn))
        {
            Details::RoTransformError(hrIn, S_OK, nullptr);
            hrIn = S_OK;
        }
        return hrIn;
    }

    static HRESULT FireProgressErrorPropagationPolicyFilter(HRESULT hrIn, IUnknown*, void* = nullptr)
    {
        if (FAILED(hrIn))
        {
            Details::RoTransformError(hrIn, S_OK, nullptr);
            hrIn = S_OK;
        }
        return hrIn;
    }
};

// All options for the AsyncBase class are accumulated here. This class may be expanded to include
// new options as needed.
template <
    ErrorPropagationPolicy errorPropagationPolicy = IgnoreDelegateError,
    PCWSTR OpName = nullptr,
    const GUID& PlatformId = GUID_NULL,
    Details::CausalitySource CausalitySource = Details::CausalitySource_Application
>
struct AsyncOptions :
    public AsyncCausalityOptions<OpName, PlatformId, CausalitySource>,
    public ErrorPropagationOptions<errorPropagationPolicy>
{
    static const bool hasCausalityOptions  = true;
    static const bool hasErrorPropagationPolicy = true;
    static const bool hasCausalityOperationName = Microsoft::WRL::Details::IsOperationName<OpName>::Value;
    static const bool isCausalityEnabled = true;
};

#pragma endregion
// End of AsyncOptions region

// AsyncBase - base class that implements the WinRT Async state machine
// this base class is designed to be used with WRL to implement an async worker object
template <
    typename TComplete,
    typename TProgress = Details::Nil,
    AsyncResultType resultType = SingleResult,
    typename TAsyncBaseOptions = AsyncOptions<>
>
class AsyncBase : public AsyncBase<TComplete, Details::Nil, resultType, TAsyncBaseOptions>
{
    typedef typename Details::ArgTraitsHelper<TProgress>::Traits ProgressTraits;
    typedef Microsoft::WRL::Details::AsyncOptionsHelper<TComplete, TAsyncBaseOptions> AllOptions;
    friend class AsyncBase<TComplete, Details::Nil, resultType, TAsyncBaseOptions>;

public:

    // since this is designed to be used inside of an RuntimeClass<> template, we can
    // only have a default constructor
    AsyncBase() :
        progressDelegate_(nullptr),
        progressDelegateBucketAssist_(nullptr)
    {
    }

    // Delegate Helpers
    STDMETHOD(PutOnProgress)(TProgress* progressHandler)
    {
        HRESULT hr = this->CheckValidStateForDelegateCall();
        if (SUCCEEDED(hr))
        {
            progressDelegate_ = progressHandler;

            if (progressDelegate_ != nullptr)
            {
                progressDelegateBucketAssist_ = Microsoft::WRL::Details::GetDelegateBucketAssist(progressDelegate_.Get());
            }

            this->TraceDelegateAssigned();
        }
        return hr;
    }

    STDMETHOD(GetOnProgress)(TProgress** progressHandler)
    {
        *progressHandler = nullptr;
        HRESULT hr = this->CheckValidStateForDelegateCall();
        if (SUCCEEDED(hr))
        {
            progressDelegate_.CopyTo(progressHandler);
        }
        return hr;
    }

    HRESULT FireProgress(const typename ProgressTraits::Arg2Type arg)
    {
        HRESULT hr = S_OK;
        ComPtr< ::IAsyncInfo> asyncInfo = this;
        ComPtr<typename Details::DerefHelper<typename ProgressTraits::Arg1Type>::DerefType> operationInterface;
        if (progressDelegate_)
        {
            hr = asyncInfo.As(&operationInterface);
            if (SUCCEEDED(hr))
            {
                this->TraceProgressNotificationStart();

                hr = progressDelegate_->Invoke(operationInterface.Get(), arg);

                this->TraceProgressNotificationComplete();
            }
        }

        // filter the errors per the Error Propagation Policy
        hr = ErrorPropagationPolicyTraits<AllOptions::PropagationPolicy>::FireProgressErrorPropagationPolicyFilter(hr, progressDelegate_.Get(), progressDelegateBucketAssist_);

        return hr;
    }

    HRESULT FireCompletion(void) override
    {
        // "this" may be deleted during the completion call. Remove progress prior to firing completion.
        progressDelegate_.Reset();
        return AsyncBase<TComplete, Details::Nil, resultType, TAsyncBaseOptions>::FireCompletion();
    }

private:
    ::Microsoft::WRL::ComPtr<TProgress> progressDelegate_;
    void* progressDelegateBucketAssist_;
};

template <typename TComplete, AsyncResultType resultType, typename TAsyncBaseOptions>
class AsyncBase<TComplete, Details::Nil, resultType, TAsyncBaseOptions> : public ::Microsoft::WRL::Implements< ::IAsyncInfo>
{
    typedef typename Details::ArgTraitsHelper<TComplete>::Traits CompleteTraits;
    typedef Microsoft::WRL::Details::AsyncOptionsHelper<TComplete, TAsyncBaseOptions> AllOptions;
public:
    // since this is designed to be used inside of a RuntimeClass<> template, we can
    // only have a default constructor
    AsyncBase() :
        cCallbackMade_(0),
        cCompleteDelegateAssigned_(0),
        completeDelegate_(nullptr),
        cCompleteDelegateRefCount_(0),
        completeDelegateBucketAssist_(nullptr),
        currentStatus_(Details::AsyncStatusInternal::_Created),
        errorCode_(S_OK),
        id_(1),
        asyncOperationBucketAssist_(nullptr)
    {
    }

    // The TraceCompletion in logged if the FireCompletion occurs and the completion call back is assigned
    // if the callback was not made then the async operation completion is logged here
    virtual ~AsyncBase()
    {
        if (!cCallbackMade_)
        {
            TraceOperationComplete();
        }
    }

    // IAsyncInfo::put_Id
    STDMETHOD(put_Id)(const unsigned int id)
    {
        if (id == 0)
        {
            static const WCHAR pszParamName[] = L"id";
            Details::RoOriginateErrorW(E_INVALIDARG, ARRAYSIZE(pszParamName) - 1, pszParamName);
            return E_INVALIDARG;
        }
        id_ = id;

        Details::AsyncStatusInternal current = Details::_Undefined;
        CurrentStatus(&current);

        if (current != Details::_Created)
        {
            Details::RoOriginateError(E_ILLEGAL_METHOD_CALL, nullptr);
            return E_ILLEGAL_METHOD_CALL;
        }

        return S_OK;
    }

    // IAsyncInfo::get_Id
    STDMETHOD(get_Id)(unsigned int* id) override
    {
        *id = id_;
        return CheckValidStateForAsyncInfoCall();
    }

    // IAsyncInfo::get_Status
    STDMETHOD(get_Status)(::AsyncStatus* status) override
    {
        Details::AsyncStatusInternal current = Details::_Undefined;
        CurrentStatus(&current);
        *status = static_cast< ::AsyncStatus>(current);
        return CheckValidStateForAsyncInfoCall();
    }

    // IAsyncInfo::get_ErrorCode
    STDMETHOD(get_ErrorCode)(HRESULT* errorCode) override
    {
        HRESULT hr = CheckValidStateForAsyncInfoCall();
        if (SUCCEEDED(hr))
        {
            ErrorCode(errorCode);
        }
        else
        {
            // Do not propagate the error and error info associated with the async error if this call generated
            // a specific error itself.
            *errorCode = hr;
        }
        return hr;
    }

protected:
    // Start - this is not externally visible since async operations "hot start" before returning to the caller
    STDMETHOD(Start)(void)
    {
        HRESULT hr = S_OK;
        if (TransitionToState(Details::_Started))
        {
            hr = OnStart();

            if (SUCCEEDED(hr))
            {
                TraceOperationStart();
            }
        }
        else
        {
            hr = E_ILLEGAL_STATE_CHANGE;
            Details::RoOriginateError(E_ILLEGAL_STATE_CHANGE, nullptr);
        }
        return hr;
    }

public:
    // IAsyncInfo::Cancel
    STDMETHOD(Cancel)(void) override
    {
        if (TransitionToState(Details::_Canceled))
        {
            OnCancel();

            TraceCancellation();
        }
        return S_OK;
    }

    // IAsyncInfo::Close
    STDMETHOD(Close)(void) override
    {
        HRESULT hr = S_OK;
        if (TransitionToState(Details::_Closed))
        {
            OnClose();
        }
        else    // illegal state change
        {
            Details::AsyncStatusInternal current = Details::_Undefined;
            CurrentStatus(&current);

            if (current == Details::_Closed)
            {
                hr = S_OK;          // Closed => Closed transition is just ignored
            }
            else
            {
                hr = E_ILLEGAL_STATE_CHANGE;
                Details::RoOriginateError(E_ILLEGAL_STATE_CHANGE, nullptr);
            }
        }
        return hr;
    }

    // Delegate helpers
    STDMETHOD(PutOnComplete)(TComplete* completeHandler)
    {
        HRESULT hr = CheckValidStateForDelegateCall();
        if (SUCCEEDED(hr))
        {
            // this delegate property is "write once"
            if (::InterlockedCompareExchange(&cCompleteDelegateAssigned_, 1, 0) == 0)
            {
                if (completeHandler != nullptr)
                {
                    completeDelegateBucketAssist_ = Microsoft::WRL::Details::GetDelegateBucketAssist(completeHandler);
                }

                completeDelegate_ = completeHandler;

                // Guarantee that the write of completeDelegate_ is ordered with respect to the read of state below
                // as perceived from FireCompletion on another thread.
                MemoryBarrier();

                // Make the write "visible" to other threads
                LONG newRefCount = ::InterlockedIncrement(&cCompleteDelegateRefCount_);
                __WRL_ASSERT__(newRefCount == 1);
                (void)newRefCount;

                this->TraceDelegateAssigned();

                // in the "hot start" case, put_Completed could have been called after the async operation has hit
                // a terminal state.  If so, fire the completion immediately.
                if (IsTerminalState())
                {
                    FireCompletion();
                }
            }
            else
            {
                hr = E_ILLEGAL_DELEGATE_ASSIGNMENT;
                Details::RoOriginateError(E_ILLEGAL_DELEGATE_ASSIGNMENT, nullptr);
            }
        }
        return hr;
    }

    STDMETHOD(GetOnComplete)(TComplete** completeHandler)
    {
        *completeHandler = nullptr;
        HRESULT hr = CheckValidStateForDelegateCall();
        if (SUCCEEDED(hr))
        {
            LONG currCount = cCompleteDelegateRefCount_;
            while (currCount != 0)
            {
                LONG oldCount = ::InterlockedCompareExchange(&cCompleteDelegateRefCount_, currCount + 1, currCount);
                if (oldCount == currCount)
                {
                    // Successfully bumped the reference count
                    break;
                }

                currCount = oldCount;
            }

            if (currCount == 0)
            {
                // Not set or not safe to read
                return hr;
            }

            // Safe to read the pointer
            completeDelegate_.CopyTo(completeHandler);

            if (::InterlockedDecrement(&cCompleteDelegateRefCount_) == 0)
            {
                // This was the last thread to release shared ownership of the pointer; we need to clean it up
                completeDelegate_ = nullptr;
            }
        }
        return hr;
    }

    virtual HRESULT FireCompletion()
    {
        HRESULT hr = S_OK;
        // must do this *before* the InterlockedCompareExchange!
        TryTransitionToCompleted();

        __WRL_ASSERT__(IsTerminalState() && "Must only call FireCompletion when operation is in terminal state");

        // we guarantee that completion can only ever be fired once
        if (completeDelegate_ != nullptr && ::InterlockedCompareExchange(&cCallbackMade_, 1, 0) == 0)
        {
            ComPtr< ::IAsyncInfo> asyncInfo = this;
            ComPtr<typename Details::DerefHelper<typename CompleteTraits::Arg1Type>::DerefType> operationInterface;

            TraceOperationComplete();

            if (SUCCEEDED(asyncInfo.As(&operationInterface)))
            {
                Details::AsyncStatusInternal current = Details::_Undefined;
                CurrentStatus(&current);

                TraceCompletionNotificationStart();

                hr = completeDelegate_->Invoke(operationInterface.Get(), static_cast< ::AsyncStatus>(current));
                // Filter the errors as per the Error Propagation Policy
                hr = ErrorPropagationPolicyTraits<AllOptions::PropagationPolicy>::FireCompletionErrorPropagationPolicyFilter(hr, completeDelegate_.Get(), completeDelegateBucketAssist_);

                if (::InterlockedDecrement(&cCompleteDelegateRefCount_) == 0)
                {
                    // No other thread is trying to read the pointer
                    completeDelegate_ = nullptr;
                }

                TraceCompletionNotificationComplete();
            }
        }

        return hr;
    }

protected:

    inline void CurrentStatus(Details::AsyncStatusInternal* status)
    {
        // Full barrier read of the current state
        *status = static_cast<Details::AsyncStatusInternal>(
            __atomic_load_n(reinterpret_cast<volatile LONG*>(&currentStatus_), __ATOMIC_SEQ_CST));
        __WRL_ASSERT__(*status != Details::_Undefined);
    }

    // This method returns the error code stored as a result of a transition into the error state.
    // In addition, if there is any restricted error information associated with the error that was captured at the time
    // of the error transition, it will be associated with the calling thread via a SetRestrictedErrorInfo call.
    inline void ErrorCode(HRESULT* error)
    {
        Details::AsyncStatusInternal current = Details::_Undefined;
        CurrentStatus(&current);

        // Do not allow visibility of the error until such point as we have had a successful state transition into the error state.
        // The error + information is not a single atomic quantity.  It is not considered "published" until we are actively in the error state.
        if (current != Details::_Error)
        {
            *error = S_OK;
        }
        else
        {
            *error = errorCode_;
            if (errorInfo_ != nullptr)
            {
                Details::SetRestrictedErrorInfo(errorInfo_.Get());
            }
        }
    }

    bool TryTransitionToCompleted(CancelTransitionPolicy cancelBehavior = CancelTransitionPolicy::RemainCanceled)
    {
        bool bTransition = TransitionToState(Details::AsyncStatusInternal::_Completed);
        if (!bTransition && cancelBehavior == CancelTransitionPolicy::TransitionFromCanceled)
        {
            bTransition = TransitionCanceledToCompleted();
        }
        return bTransition;
    }

    bool TryTransitionToError(const HRESULT error, CancelTransitionPolicy cancelBehavior = CancelTransitionPolicy::RemainCanceled, void* bucketAssist = nullptr)
    {
        // In addition to the result being transitioned to, there might be restricted error information associated with the error.  It
        // is assumed that such is on the calling thread.  If we successfully transition to the error state with "error" as the code,
        // we must also capture the error info and funnel it over to callers of GetResults / ErrorCode.  Our call to
        // GetRestrictedErrorInfo below will capture the error info after which, it is owned by this async operation.
        //
        // Since there are multiple pieces of information and the capturing of these are not atomic, no one from the outside is allowed
        // to view these until the state transition to error is complete.  This happens in two parts:
        //
        // - A successful CAS from S_OK to error (meaning that this error is the one being captures)
        // - A successful state change into the error state (via the Transition* call below)

        if (bucketAssist != nullptr)
        {
            asyncOperationBucketAssist_ = bucketAssist;
        }
        bool bTransition = false;
        if (::InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&errorCode_), error, S_OK) == S_OK)
        {
            (void)Details::GetRestrictedErrorInfo(errorInfo_.ReleaseAndGetAddressOf());

            // This thread is the "owner" of the rights to transition to the error state.
            bTransition = TransitionToState(Details::AsyncStatusInternal::_Error);
            if (!bTransition && cancelBehavior == CancelTransitionPolicy::TransitionFromCanceled)
            {
                bTransition = TransitionCanceledToError();
            }
        }

        if (bTransition)
        {
            TraceError();
        }

        // if we return true, then we did a valid state transition
        // queue firing of completed event (cannot be done from this call frame)
        // otherwise we are already in a terminal state: error, canceled, completed, or closed
        // and we ignore the transition request to the Error state
        return bTransition;
    }

    // This method checks to see if the delegate properties can be
    // modified in the current state and generates the appropriate
    // error hr in the case of violation.
    inline HRESULT CheckValidStateForDelegateCall()
    {
        Details::AsyncStatusInternal current = Details::_Undefined;
        CurrentStatus(&current);
        if (current == Details::_Closed)
        {
            Details::RoOriginateError(E_ILLEGAL_METHOD_CALL, nullptr);
            return E_ILLEGAL_METHOD_CALL;
        }
        return S_OK;
    }

    // This method checks to see if results can be collected in the
    // current state and generates the appropriate error hr in
    // the case of a violation.
    inline HRESULT CheckValidStateForResultsCall()
    {
        Details::AsyncStatusInternal current = Details::_Undefined;
        CurrentStatus(&current);

        if (current == Details::_Error)
        {
            HRESULT hr;

            // Make sure to propagate any restricted error info associated with the asynchronous failure.
            ErrorCode(&hr);
            return hr;
        }
        // single result only legal in Completed state
        if (resultType == SingleResult)
        {
            if (current != Details::_Completed)
            {
                Details::RoOriginateError(E_ILLEGAL_METHOD_CALL, nullptr);
                return E_ILLEGAL_METHOD_CALL;
            }
        }
        // multiple results can be called after async operation is running (started) and before/after Completed
        else if (current != Details::_Started &&
                 current != Details::_Canceled &&
                 current != Details::_Completed)
        {
            Details::RoOriginateError(E_ILLEGAL_METHOD_CALL, nullptr);
            return E_ILLEGAL_METHOD_CALL;
        }
        return S_OK;
    }

    // This method can be called by derived classes periodically to determine
    // whether the asynchronous operation should continue processing or should
    // be halted.
    inline bool ContinueAsyncOperation()
    {
        Details::AsyncStatusInternal current = Details::_Undefined;
        CurrentStatus(&current);
        return (current == Details::_Started);
    }

    // These methods are used to allow the async worker implementation do work on
    // state transitions. No real "work" should be done in these methods. In other words
    // they should not block for a long time on UI timescales.
    virtual HRESULT OnStart(void) = 0;
    virtual void OnClose(void) = 0;
    virtual void OnCancel(void) = 0;

private:
    // This method is used to check if calls to the AsyncInfo properties
    // (id, status, error code) are legal in the current state. It also
    // generates the appropriate error hr to return in the case of an
    // illegal call.
    inline HRESULT CheckValidStateForAsyncInfoCall()
    {
        Details::AsyncStatusInternal current = Details::_Undefined;
        CurrentStatus(&current);
        if (current == Details::_Closed)
        {
            Details::RoOriginateError(E_ILLEGAL_METHOD_CALL, nullptr);
            return E_ILLEGAL_METHOD_CALL;
        }
        else if (current == Details::_Created)  // error in async ::ABI object - returned to caller not started
        {
            // No RoOriginateError needed since this can hit multiple times in expected scenarios

            return E_ASYNC_OPERATION_NOT_STARTED;
        }

        return S_OK;
    }

    inline bool TransitionToState(const Details::AsyncStatusInternal newState)
    {
        Details::AsyncStatusInternal current = Details::_Undefined;
        CurrentStatus(&current);

        // This enforces the valid state transitions of the asynchronous worker object
        // state machine.
        switch (newState)
        {
        case Details::_Started:
            if (current != Details::_Created)
            {
                return false;
            }
            break;
        case Details::_Completed:
            if (current != Details::_Started)
            {
                return false;
            }
            break;
        case Details::_Canceled:
            if (current != Details::_Started)
            {
                return false;
            }
            break;
        case Details::_Error:
            if (current != Details::_Started)
            {
                return false;
            }
            break;
        case Details::_Closed:
            if (!IsTerminalState(current))
            {
                return false;
            }
            break;
        default:
            return false;
        }
        // attempt the transition to the new state
        // Note: if currentStatus_ == current, then there was no intervening write
        // by the async work object and the swap succeeded.
        Details::AsyncStatusInternal retState = static_cast<Details::AsyncStatusInternal>(
                ::InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&currentStatus_),
                                             newState,
                                             static_cast<LONG>(current)));

        // ICE returns the former state, if the returned state and the
        // state we captured at the beginning of this method are the same,
        // the swap succeeded.
        return (retState == current);
    }

protected:

    // It is legal for an async operation object to transition from (client-requested) Canceled
    // state to Completed if, for example, the operation completed near the time of the cancellation request.
    // An operation which is no longer responsive to client requests to cancel and intends to complete
    // successfully despite any new incoming requests to cancel should call TryTransitionToCompleted and
    // pass TransitionFromCanceled instead of using this method.
    inline bool TransitionCanceledToCompleted()
    {
        // this is somewhat overly pessimistic since the client cannot possibly transition
        // the operation out of the canceled state (only the async operation itself can call
        // this method)
        Details::AsyncStatusInternal retState = static_cast<Details::AsyncStatusInternal>(
                ::InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&currentStatus_),
                                             Details::AsyncStatusInternal::_Completed,
                                             Details::AsyncStatusInternal::_Canceled));
        return (retState == Details::AsyncStatusInternal::_Canceled);
    }

    // It is legal for an async operation object to transition from (client-requested) Canceled
    // state to the error state if, for example, the operation encountered an error near the time
    // of the cancellation request.  An operation which is no longer responsive to client requests to cancel
    // and intends to complete with an error despite any new incoming requests to cancel should call
    // TryTransitionToError and pass TransitionFromCanceled instead of using this method.
    inline bool TransitionCanceledToError()
    {
        Details::AsyncStatusInternal retState = static_cast<Details::AsyncStatusInternal>(
                ::InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&currentStatus_),
                                             Details::AsyncStatusInternal::_Error,
                                             Details::AsyncStatusInternal::_Canceled));
        return (retState == Details::AsyncStatusInternal::_Canceled);
    }

    inline bool IsTerminalState()
    {
        Details::AsyncStatusInternal current = Details::_Undefined;
        CurrentStatus(&current);
        return IsTerminalState(current);
    }

    inline bool IsTerminalState(Details::AsyncStatusInternal status)
    {
        return (status == Details::_Error ||
                status == Details::_Canceled ||
                status == Details::_Completed ||
                status == Details::_Closed);
    }

    LONG volatile cCallbackMade_;
    LONG volatile cCompleteDelegateAssigned_;

#pragma region TracingMethods
    // Causality tracing is not available on mingw-w64. The methods are kept so
    // that derived classes calling them still compile.
    void TraceOperationStart() {}
    void TraceOperationComplete() {}
    void TraceProgressNotificationStart() {}
    void TraceProgressNotificationComplete() {}
    void TraceCompletionNotificationStart() {}
    void TraceCompletionNotificationComplete() {}
    void TraceExecutionStart(int /*traceLevel*/) {}
    void TraceExecutionComplete(int /*traceLevel*/) {}
    void TraceDelegateAssigned() {}
    void TraceError() {}
    void TraceCancellation() {}
#pragma endregion

private:
    ::Microsoft::WRL::ComPtr<TComplete> completeDelegate_;
    LONG volatile cCompleteDelegateRefCount_;
    void* completeDelegateBucketAssist_;

    ::Microsoft::WRL::ComPtr<IUnknown> errorInfo_;  // IRestrictedErrorInfo
    Details::AsyncStatusInternal volatile currentStatus_;
    HRESULT volatile errorCode_;
    unsigned int id_;

protected:
    void* asyncOperationBucketAssist_;
};

}} // namespace Microsoft::WRL

#define CAUSALITY_OPTIONS(OpName) \
    Microsoft::WRL::AsyncCausalityOptions< OpName >
#define ASYNCBASE_CAUSALITY_OPTIONS(TComplete, OpName) \
    Microsoft::WRL::AsyncBase< TComplete, Microsoft::WRL::Details::Nil, Microsoft::WRL::AsyncResultType::SingleResult, CAUSALITY_OPTIONS(OpName)>
#define ASYNCBASE_WITH_PROGRESS_CAUSALITY_OPTIONS(TComplete, TProgress, OpName) \
    Microsoft::WRL::AsyncBase< TComplete, TProgress, Microsoft::WRL::AsyncResultType::SingleResult, CAUSALITY_OPTIONS(OpName)>
#define ASYNCBASE_DISABLE_CAUSALITY(TComplete) \
    Microsoft::WRL::AsyncBase< TComplete, Microsoft::WRL::Details::Nil, Microsoft::WRL::AsyncResultType::SingleResult, Microsoft::WRL::DisableCausality>
#define ASYNCBASE_WITH_PROGRESS_DISABLE_CAUSALITY(TComplete, TProgress) \
    Microsoft::WRL::AsyncBase< TComplete, TProgress, Microsoft::WRL::AsyncResultType::SingleResult, Microsoft::WRL::DisableCausality>

#endif // _WRL_ASYNC_H_
