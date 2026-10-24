/**
 * wrl/implements.h - Microsoft::WRL RuntimeClass / Implements for mingw-w64.
 *
 * Port of the Windows SDK <wrl/implements.h> to mingw-w64 (clang/gcc, Itanium
 * C++ ABI, no MS extensions). Class layout, QueryInterface semantics, reference
 * counting and weak reference support follow the SDK implementation.
 *
 * Differences from the SDK header:
 *  - The linker generated creator map used by <wrl/module.h> (ActivatableClass,
 *    CoCreatableClass, ...) lives in ".CRT$XCZ$WRL$*" sections instead of the
 *    SDK "minATL$*" sections, see ModuleBase below.
 *  - RoOriginateError / RoTransformError (and the restricted error info API in
 *    <wrl/async.h>) are not declared by mingw-w64 headers; they are resolved
 *    from combase.dll at runtime, so no runtimeobject import library is needed.
 *  - WeakRef / AsWeak and AgileRef / AsAgile live here because mingw-w64
 *    <wrl/client.h> lacks them.
 */

#ifndef _WRL_IMPLEMENTS_H_
#define _WRL_IMPLEMENTS_H_

#if __cplusplus < 201703L
#error <wrl/implements.h> for mingw-w64 requires C++17 or later
#endif

#include <windows.h>
#include <unknwn.h>
#include <objbase.h>     // IMarshal, CoCreateFreeThreadedMarshaler
#include <objidl.h>      // IAgileObject, IGlobalInterfaceTable
#include <cguid.h>       // CLSID_StdGlobalInterfaceTable
#include <inspectable.h>
#include <roapi.h>
#include <activation.h>
#include <winstring.h>
#include <weakreference.h>

#include <assert.h>
#include <limits.h>
#include <wchar.h>
#include <new>
#include <type_traits>
#include <utility>

#include <wrl/internal.h>
#include <wrl/client.h>

#ifndef __WRL_ASSERT__
#define __WRL_ASSERT__(cond) assert(cond)
#endif

#ifndef __WRL_IMPLEMENTS_FTM_BASE__
#ifdef __WRL_CONFIGURATION_LEGACY__
#define __WRL_IMPLEMENTS_FTM_BASE__(flags) (false)
#else
#define __WRL_IMPLEMENTS_FTM_BASE__(flags) ((flags & ::Microsoft::WRL::InhibitFtmBase) == 0)
#endif
#endif

#if defined(__clang__)
#pragma clang diagnostic push
// An interface listed after IInspectable/IUnknown makes the direct base
// ambiguous; WRL reaches it through reinterpret_cast on purpose.
#pragma clang diagnostic ignored "-Winaccessible-base"
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#pragma clang diagnostic ignored "-Wdelete-non-abstract-non-virtual-dtor"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Winaccessible-base"
#pragma GCC diagnostic ignored "-Wsubobject-linkage"
#endif

namespace Microsoft {
namespace WRL {

// Indicator for RuntimeClass, Implements and ChainInterfaces that T interface
// will be not accessible on IID list
// Example:
// struct MyRuntimeClass : RuntimeClass<CloakedIid<IMyCloakedInterface>> {}
template <typename T>
struct CloakedIid : T
{
};

enum RuntimeClassType
{
    WinRt                   = 0x0001,
    ClassicCom              = 0x0002,
    WinRtClassicComMix      = WinRt | ClassicCom,
    InhibitWeakReference    = 0x0004,
    Delegate                = ClassicCom,
    InhibitFtmBase          = 0x0008,
    InhibitRoOriginateError = 0x0010
};

template <unsigned int flags>
struct RuntimeClassFlags
{
    static constexpr unsigned int value = flags;
};

namespace Details
{
// Empty struct used for validating template parameter types in Implements
struct ImplementsBase
{
};

// mingw-w64 does not declare the RoOriginateError family, resolve lazily.
inline FARPROC GetCombaseProc(const char* name) noexcept
{
    HMODULE combase = ::GetModuleHandleW(L"combase.dll");
    if (combase == nullptr)
    {
        combase = ::LoadLibraryExW(L"combase.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    }
    return combase != nullptr ? ::GetProcAddress(combase, name) : nullptr;
}

inline BOOL RoOriginateError(HRESULT error, HSTRING message) noexcept
{
    typedef BOOL (WINAPI *Fn)(HRESULT, HSTRING);
    static const Fn fn = reinterpret_cast<Fn>(reinterpret_cast<void*>(GetCombaseProc("RoOriginateError")));
    return fn != nullptr ? fn(error, message) : FALSE;
}

inline BOOL RoOriginateErrorW(HRESULT error, UINT cchMax, PCWSTR message) noexcept
{
    typedef BOOL (WINAPI *Fn)(HRESULT, UINT, PCWSTR);
    static const Fn fn = reinterpret_cast<Fn>(reinterpret_cast<void*>(GetCombaseProc("RoOriginateErrorW")));
    return fn != nullptr ? fn(error, cchMax, message) : FALSE;
}

inline BOOL RoTransformError(HRESULT oldError, HRESULT newError, HSTRING message) noexcept
{
    typedef BOOL (WINAPI *Fn)(HRESULT, HRESULT, HSTRING);
    static const Fn fn = reinterpret_cast<Fn>(reinterpret_cast<void*>(GetCombaseProc("RoTransformError")));
    return fn != nullptr ? fn(oldError, newError, message) : FALSE;
}

} // namespace Details

// MixIn modifier allows to combine QI from
// a class that doesn't have default constructor on it
template <typename Derived, typename MixInType, bool hasImplements = std::is_base_of<Details::ImplementsBase, MixInType>::value>
struct MixIn
{
};

// ComposableBase template to allow deriving from a RuntimeClass
// Optionally allows specifying the base factory and statics interface
template <typename FactoryInterface = IInspectable>
class ComposableBase
{
};

// Back-compat indicator for RuntimeClass to not support IWeakReferenceSource
typedef RuntimeClassFlags<WinRt | InhibitWeakReference> InhibitWeakReferencePolicy;

template <unsigned int RuntimeClassTypeT>
struct ErrorHelper
{
    static void OriginateError(HRESULT hr, HSTRING message) noexcept
    {
        ::Microsoft::WRL::Details::RoOriginateError(hr, message);
    }
};

template <>
struct ErrorHelper<InhibitRoOriginateError>
{
    static void OriginateError(HRESULT, HSTRING) noexcept
    {
        // No-Op
    }
};

namespace Details
{

//Forward declaration
struct CreatorMap;

// Linker generated list of pointers to CreatorMap (see <wrl/module.h>).
//
// The SDK uses "minATL$__a".."minATL$__z" grouped sections. On mingw-w64 the
// list lives in ".CRT$XCZ$WRL$<group>" sections instead, because:
//  - lld (MinGW mode) sorts grouped sections by name only for ".CRT$*", and
//    GNU ld sorts and KEEPs ".CRT$XC*" even with --gc-sections;
//  - ".CRT$XCZ$..." sorts after mingw-w64's __xc_z terminator, so the CRT
//    never treats the entries as C++ initializers.
// Groups: __a first marker, __f COM entries, __m middle marker, __r WinRT
// entries, __z last marker.
#define __WRL_CREATOR_MAP_SECTION__(group) __attribute__((section(".CRT$XCZ$WRL$" group), used))

extern "C"
{
// Location of the first and last entries for the linker generated list of pointers to CreatorMapEntry
__WRL_CREATOR_MAP_SECTION__("__a") inline const CreatorMap* const __pobjectentryfirst = nullptr;
// Section m divides COM objects from WinRT objects
// - sections between a and m we store COM object info
// - sections between m+1 and z we store WinRT object info
__WRL_CREATOR_MAP_SECTION__("__m") inline const CreatorMap* const __pobjectentrymid = nullptr;
__WRL_CREATOR_MAP_SECTION__("__z") inline const CreatorMap* const __pobjectentrylast = nullptr;
}

// Walking from one marker to another is undefined behavior in C++ terms (the
// markers are distinct objects). Hide the pointer provenance from the
// optimizer, like the SDK implicitly relies on with MSVC.
inline const CreatorMap** LaunderCreatorMapPointer(const CreatorMap* const* entry) noexcept
{
    const CreatorMap** result = const_cast<const CreatorMap**>(entry);
    __asm__("" : "+r"(result));
    return result;
}

// Base class used by all module classes.
class ModuleBase
{
private:
    // Lock that synchronize access and termination of factories
    static inline SRWLOCK moduleLock_ = SRWLOCK_INIT;

protected:
    static inline volatile unsigned long objectCount_ = 0;

public:
    static inline ModuleBase* module_ = nullptr;

    ModuleBase() noexcept
    {
#ifdef _DEBUG
        // WRLs support for activatable classes requires there is only one instance of Module<>, this assert
        // ensures there is only one. Since Module<> is templatized, using different template parameters will
        // result in multiple instances, avoid this by making sure all code in a component uses the same parameters.
        __WRL_ASSERT__(::InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(&module_), this, nullptr) == nullptr &&
            "The module was already instantiated");
#else
        module_ = this;
#endif
    }

    ModuleBase(const ModuleBase&) = delete;
    ModuleBase& operator=(const ModuleBase&) = delete;

    virtual ~ModuleBase() noexcept
    {
#ifdef _DEBUG
        __WRL_ASSERT__(::InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(&module_), nullptr, this) == this &&
            "The module was already instantiated");
#else
        module_ = nullptr;
#endif
    }

    // Number of active objects in the module
    STDMETHOD_(unsigned long, IncrementObjectCount)() = 0;
    STDMETHOD_(unsigned long, DecrementObjectCount)() = 0;

    STDMETHOD_(unsigned long, GetObjectCount)() const
    {
        return objectCount_;
    }

    STDMETHOD_(const CreatorMap**, GetFirstEntryPointer)() const
    {
        return LaunderCreatorMapPointer(&__pobjectentryfirst);
    }

    STDMETHOD_(const CreatorMap**, GetMidEntryPointer)() const
    {
        return LaunderCreatorMapPointer(&__pobjectentrymid);
    }

    STDMETHOD_(const CreatorMap**, GetLastEntryPointer)() const
    {
        return LaunderCreatorMapPointer(&__pobjectentrylast);
    }

    STDMETHOD_(SRWLOCK*, GetLock)() const
    {
        return &moduleLock_;
    }

    STDMETHOD(RegisterWinRTObject)(const wchar_t*, const wchar_t** activatableClassIds, RO_REGISTRATION_COOKIE* cookie, unsigned int) = 0;
    STDMETHOD(UnregisterWinRTObject)(const wchar_t*, RO_REGISTRATION_COOKIE) = 0;
    STDMETHOD(RegisterCOMObject)(const wchar_t*, IID*, IClassFactory**, DWORD*, unsigned int) = 0;
    STDMETHOD(UnregisterCOMObject)(const wchar_t*, DWORD*, unsigned int) = 0;
};

// mingw-w64 declares RO_REGISTRATION_COOKIE as a pointer to an unnamed struct
// (a type without linkage). GCC rejects calls to undefined functions using such
// a type, so the pure virtuals get (never called) definitions.
inline COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE ModuleBase::RegisterWinRTObject(const wchar_t*, const wchar_t**, RO_REGISTRATION_COOKIE*, unsigned int)
{
    return E_NOTIMPL;
}

inline COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE ModuleBase::UnregisterWinRTObject(const wchar_t*, RO_REGISTRATION_COOKIE)
{
    return E_NOTIMPL;
}

#pragma region helper types
// Empty struct used as default template parameter
class Nil
{
};

// Used on RuntimeClass to protect it from being constructed with new
class DontUseNewUseMake
{
private:
    void* operator new(size_t) noexcept
    {
        __WRL_ASSERT__(false);
        return nullptr;
    }

public:
    void* operator new(size_t, void* placement) noexcept
    {
        return placement;
    }
};

// RuntimeClassBase is used for detection of RuntimeClass in Make method
class RuntimeClassBase
{
};

// RuntimeClassBaseT provides helper methods for QI and getting IIDs
template <unsigned int RuntimeClassTypeT>
class RuntimeClassBaseT : private RuntimeClassBase
{
protected:
    template <typename T>
    static HRESULT AsIID(T* implements, REFIID riid, void** ppvObject) noexcept
    {
        *ppvObject = nullptr;
        bool isRefDelegated = false;
        // Prefer InlineIsEqualGUID over other forms since it's better perf on 4-byte aligned data, which is almost always the case.
        if (InlineIsEqualGUID(riid, __uuidof(IUnknown)) ||
            ((RuntimeClassTypeT & WinRt) != 0 && InlineIsEqualGUID(riid, __uuidof(IInspectable))))
        {
            *ppvObject = implements->CastToUnknown();
            static_cast<IUnknown*>(*ppvObject)->AddRef();
            return S_OK;
        }

        HRESULT hr = implements->CanCastTo(riid, ppvObject, &isRefDelegated);
        if (SUCCEEDED(hr) && !isRefDelegated)
        {
            static_cast<IUnknown*>(*ppvObject)->AddRef();
        }

        return hr;
    }

    template <typename T>
    static HRESULT GetImplementedIIDS(T* implements, ULONG* iidCount, IID** iids) noexcept
    {
        *iids = nullptr;
        *iidCount = 0;
        unsigned long count = implements->GetIidCount();

        // If there is no iids the CoTaskMemAlloc don't have to be called
        if (count == 0)
        {
            return S_OK;
        }

        IID* iidArray = reinterpret_cast<IID*>(::CoTaskMemAlloc(sizeof(IID) * count));
        if (iidArray == nullptr)
        {
            return E_OUTOFMEMORY;
        }

        unsigned long index = 0;

        // assign the IIDs to the array
        implements->FillArrayWithIid(&index, iidArray);
        __WRL_ASSERT__(index == count);

        // and return it
        *iidCount = count;
        *iids = iidArray;
        return S_OK;
    }

public:
    HRESULT RuntimeClassInitialize() noexcept
    {
        return S_OK;
    }
};

// Base class required to mark FtmBase
class FtmBaseMarker
{
};

// Verifies that I is derived from specified base
template <unsigned int type, typename I, bool doStrictCheck = true, bool isImplementsBased = std::is_base_of<ImplementsBase, I>::value>
struct VerifyInterfaceHelper;

// Specialization for ClassicCom interface
template <typename I, bool doStrictCheck>
struct VerifyInterfaceHelper<ClassicCom, I, doStrictCheck, false>
{
    static void Verify() noexcept
    {
#ifdef __WRL_STRICT__
        // Make sure that your interfaces inherit from IUnknown and are not IUnknown and/or IInspectable based
        // The IUnknown is allowed only on RuntimeClass as first template parameter
        static_assert(__is_base_of(IUnknown, I) && !__is_base_of(IInspectable, I) && !(doStrictCheck && std::is_same<IUnknown, I>::value),
            "'I' has to derive from 'IUnknown' and not from 'IInspectable'. 'I' must not be IUnknown.");
#else
        static_assert(__is_base_of(IUnknown, I), "'I' has to derive from 'IUnknown'.");
#endif
    }
};

// Specialization for WinRtClassicComMix interface
template <typename I, bool doStrictCheck>
struct VerifyInterfaceHelper<WinRtClassicComMix, I, doStrictCheck, false>
{
    static void Verify() noexcept
    {
#ifdef __WRL_STRICT__
        // Make sure that your interfaces inherit from IUnknown and are not IUnknown and/or IInspectable
        // except when IInspectable is the first template parameter
        static_assert(__is_base_of(IUnknown, I) &&
            (doStrictCheck ? !(std::is_same<IInspectable, I>::value || std::is_same<IUnknown, I>::value) : __is_base_of(IInspectable, I)),
                "'I' has to derive from 'IUnknown' and must not be IUnknown and/or IInspectable.");
#else
        static_assert(__is_base_of(IUnknown, I), "'I' has to derive from 'IUnknown'.");
#endif
    }
};

// Specialization for WinRt interface
template <typename I, bool doStrictCheck>
struct VerifyInterfaceHelper<WinRt, I, doStrictCheck, false>
{
    static void Verify() noexcept
    {
#ifdef __WRL_STRICT__
        // IWeakReferenceSource is exception for WinRt and can be used however it cannot be first templated interface
        // Make sure that your interfaces inherit from IInspectable and are not IInspectable
        // The IInspectable is allowed only on RuntimeClass as first template parameter
        static_assert((__is_base_of(IWeakReferenceSource, I) && doStrictCheck) ||
            (__is_base_of(IInspectable, I) && !(doStrictCheck && std::is_same<IInspectable, I>::value)),
                "'I' has to derive from 'IWeakReferenceSource' or 'IInspectable' and must not be IInspectable");
#else
        // IWeakReference and IWeakReferneceSource are exceptions for WinRT
        static_assert(__is_base_of(IWeakReference, I) ||
                        __is_base_of(IWeakReferenceSource, I) ||
                            __is_base_of(IInspectable, I), "'I' has to derive from 'IWeakReference', 'IWeakReferenceSource' or 'IInspectable'");
#endif
    }
};

// Specialization for Implements passed as template parameter
template <unsigned int type, typename I>
struct VerifyInterfaceHelper<type, I, true, true>
{
    static void Verify() noexcept
    {
#ifdef __WRL_STRICT__
        // Verifies if Implements has correct RuntimeClassFlags setting
        // Allow using FtmBase on classes configured with RuntimeClassFlags<WinRt> (Default configuration)
        static_assert(I::ClassFlags::value == type ||
                type == WinRtClassicComMix ||
                    __is_base_of(::Microsoft::WRL::Details::FtmBaseMarker, I),
            "Implements class must have the same and/or compatibile flags configuration");
#endif
    }
};

// Specialization for Implements passed as first template parameter
template <unsigned int type, typename I>
struct VerifyInterfaceHelper<type, I, false, true>
{
    static void Verify() noexcept
    {
#ifdef __WRL_STRICT__
        // Verifies if Implements has correct RuntimeClassFlags setting
        static_assert(I::ClassFlags::value == type || type == WinRtClassicComMix,
            "Implements class must have the same and/or compatible flags configuration."
                "If you use WRL::FtmBase it cannot be specified as first template parameter on RuntimeClass");

        // Besides make sure that the first interface on Implements meet flags requirement
        VerifyInterfaceHelper<type, typename I::FirstInterface, false>::Verify();
#endif
    }
};

// Interface traits provides casting and filling iids methods helpers
template <typename I0>
struct InterfaceTraits
{
    typedef I0 Base;
    static const unsigned long IidCount = 1;

    template <unsigned int ClassType>
    static void Verify() noexcept
    {
        VerifyInterfaceHelper<ClassType & WinRtClassicComMix, Base>::Verify();
    }

    template <typename T>
    static Base* CastToBase(T* ptr) noexcept
    {
        return static_cast<Base*>(ptr);
    }

    template <typename T>
    static IUnknown* CastToUnknown(T* ptr) noexcept
    {
        return static_cast<IUnknown*>(static_cast<Base*>(ptr));
    }

    template <typename T>
    static bool CanCastTo(T* ptr, REFIID riid, void** ppv) noexcept
    {
        // Prefer InlineIsEqualGUID over other forms since it's better perf on 4-byte aligned data, which is almost always the case.
        if (InlineIsEqualGUID(riid, __uuidof(Base)))
        {
            *ppv = static_cast<Base*>(ptr);
            return true;
        }

        return false;
    }

    static void FillArrayWithIid(unsigned long* index, IID* iids) noexcept
    {
        *(iids + *index) = __uuidof(Base);
        (*index)++;
    }
};

// Specialization of traits for cloaked interface
template <typename CloakedType>
struct InterfaceTraits<CloakedIid<CloakedType>>
{
    typedef CloakedType Base;
    static const unsigned long IidCount = 0;

    template <unsigned int ClassType>
    static void Verify() noexcept
    {
        VerifyInterfaceHelper<ClassType & WinRtClassicComMix, Base>::Verify();
    }

    template <typename T>
    static Base* CastToBase(T* ptr) noexcept
    {
        return static_cast<Base*>(ptr);
    }

    template <typename T>
    static IUnknown* CastToUnknown(T* ptr) noexcept
    {
        return static_cast<IUnknown*>(static_cast<Base*>(ptr));
    }

    template <typename T>
    static bool CanCastTo(T* ptr, REFIID riid, void** ppv) noexcept
    {
        // Prefer InlineIsEqualGUID over other forms since it's better perf on 4-byte aligned data, which is almost always the case.
        if (InlineIsEqualGUID(riid, __uuidof(Base)))
        {
            *ppv = static_cast<Base*>(ptr);
            return true;
        }

        return false;
    }

    // Cloaked specialization makes it always IID list empty
    static void FillArrayWithIid(unsigned long*, IID*) noexcept
    {
    }
};

// Specialization for Nil parameter
template <>
struct InterfaceTraits<Nil>
{
    typedef Nil Base;
    static const unsigned long IidCount = 0;

    template <unsigned int ClassType>
    static void Verify() noexcept
    {
    }

    static void FillArrayWithIid(unsigned long*, IID*) noexcept
    {
    }

    template <typename T>
    static bool CanCastTo(T*, REFIID, void**) noexcept
    {
        return false;
    }
};

// Verify inheritance
template <typename I, typename Base>
struct VerifyInheritanceHelper
{
    static void Verify() noexcept
    {
        typedef typename InterfaceTraits<Base>::Base BaseInterface;
        typedef typename InterfaceTraits<I>::Base DerivedInterface;
        static_assert(__is_base_of(BaseInterface, DerivedInterface) && !std::is_same<BaseInterface, DerivedInterface>::value,
            "'I' needs to inherit from 'Base'.");
    }
};

template <typename I>
struct VerifyInheritanceHelper<I, Nil>
{
    static void Verify() noexcept
    {
    }
};

#pragma endregion //  helper types

} // namespace Details

// note: Due to potential shutdown ordering issues, the results of GetModuleBase
// should always be checked for null on reference counting and cleanup operations.
inline Details::ModuleBase* GetModuleBase() noexcept
{
    return Details::ModuleBase::module_;
}

// ChainInterfaces - template allows specifying a derived COM interface along with its class hierarchy to allow QI for the base interfaces
template <typename I0, typename I1, typename I2 = Details::Nil, typename I3 = Details::Nil,
        typename I4 = Details::Nil, typename I5 = Details::Nil, typename I6 = Details::Nil,
        typename I7 = Details::Nil, typename I8 = Details::Nil, typename I9 = Details::Nil>
struct ChainInterfaces : I0
{
protected:
    template <unsigned int ClassType>
    static void Verify() noexcept
    {
        Details::InterfaceTraits<I0>::template Verify<ClassType>();
        Details::InterfaceTraits<I1>::template Verify<ClassType>();
        Details::InterfaceTraits<I2>::template Verify<ClassType>();
        Details::InterfaceTraits<I3>::template Verify<ClassType>();
        Details::InterfaceTraits<I4>::template Verify<ClassType>();
        Details::InterfaceTraits<I5>::template Verify<ClassType>();
        Details::InterfaceTraits<I6>::template Verify<ClassType>();
        Details::InterfaceTraits<I7>::template Verify<ClassType>();
        Details::InterfaceTraits<I8>::template Verify<ClassType>();
        Details::InterfaceTraits<I9>::template Verify<ClassType>();

        Details::VerifyInheritanceHelper<I0, I1>::Verify();
        Details::VerifyInheritanceHelper<I0, I2>::Verify();
        Details::VerifyInheritanceHelper<I0, I3>::Verify();
        Details::VerifyInheritanceHelper<I0, I4>::Verify();
        Details::VerifyInheritanceHelper<I0, I5>::Verify();
        Details::VerifyInheritanceHelper<I0, I6>::Verify();
        Details::VerifyInheritanceHelper<I0, I7>::Verify();
        Details::VerifyInheritanceHelper<I0, I8>::Verify();
        Details::VerifyInheritanceHelper<I0, I9>::Verify();
    }

    HRESULT CanCastTo(REFIID riid, void** ppv) noexcept
    {
        typename Details::InterfaceTraits<I0>::Base* ptr = Details::InterfaceTraits<I0>::CastToBase(this);

        return (Details::InterfaceTraits<I0>::CanCastTo(this, riid, ppv) ||
            Details::InterfaceTraits<I1>::CanCastTo(ptr, riid, ppv) ||
            Details::InterfaceTraits<I2>::CanCastTo(ptr, riid, ppv) ||
            Details::InterfaceTraits<I3>::CanCastTo(ptr, riid, ppv) ||
            Details::InterfaceTraits<I4>::CanCastTo(ptr, riid, ppv) ||
            Details::InterfaceTraits<I5>::CanCastTo(ptr, riid, ppv) ||
            Details::InterfaceTraits<I6>::CanCastTo(ptr, riid, ppv) ||
            Details::InterfaceTraits<I7>::CanCastTo(ptr, riid, ppv) ||
            Details::InterfaceTraits<I8>::CanCastTo(ptr, riid, ppv) ||
            Details::InterfaceTraits<I9>::CanCastTo(ptr, riid, ppv)) ? S_OK : E_NOINTERFACE;
    }

    IUnknown* CastToUnknown() noexcept
    {
        return Details::InterfaceTraits<I0>::CastToUnknown(this);
    }

    static const unsigned long IidCount =
        Details::InterfaceTraits<I0>::IidCount +
        Details::InterfaceTraits<I1>::IidCount +
        Details::InterfaceTraits<I2>::IidCount +
        Details::InterfaceTraits<I3>::IidCount +
        Details::InterfaceTraits<I4>::IidCount +
        Details::InterfaceTraits<I5>::IidCount +
        Details::InterfaceTraits<I6>::IidCount +
        Details::InterfaceTraits<I7>::IidCount +
        Details::InterfaceTraits<I8>::IidCount +
        Details::InterfaceTraits<I9>::IidCount;

    static void FillArrayWithIid(unsigned long* index, IID* iids) noexcept
    {
        Details::InterfaceTraits<I0>::FillArrayWithIid(index, iids);
        Details::InterfaceTraits<I1>::FillArrayWithIid(index, iids);
        Details::InterfaceTraits<I2>::FillArrayWithIid(index, iids);
        Details::InterfaceTraits<I3>::FillArrayWithIid(index, iids);
        Details::InterfaceTraits<I4>::FillArrayWithIid(index, iids);
        Details::InterfaceTraits<I5>::FillArrayWithIid(index, iids);
        Details::InterfaceTraits<I6>::FillArrayWithIid(index, iids);
        Details::InterfaceTraits<I7>::FillArrayWithIid(index, iids);
        Details::InterfaceTraits<I8>::FillArrayWithIid(index, iids);
        Details::InterfaceTraits<I9>::FillArrayWithIid(index, iids);
    }
};

template <typename DerivedType, typename BaseType, bool hasImplements, typename I1, typename I2, typename I3,
        typename I4, typename I5, typename I6,
        typename I7, typename I8, typename I9>
struct ChainInterfaces<MixIn<DerivedType, BaseType, hasImplements>, I1, I2, I3, I4, I5, I6, I7, I8, I9>
{
    static_assert(!hasImplements, "Cannot use ChainInterfaces<MixIn<...>> to Mix a class implementing interfaces using \"Implements\"");

protected:
    template <unsigned int ClassType>
    static void Verify() noexcept
    {
        Details::InterfaceTraits<BaseType>::template Verify<ClassType>();
        Details::InterfaceTraits<I1>::template Verify<ClassType>();
        Details::InterfaceTraits<I2>::template Verify<ClassType>();
        Details::InterfaceTraits<I3>::template Verify<ClassType>();
        Details::InterfaceTraits<I4>::template Verify<ClassType>();
        Details::InterfaceTraits<I5>::template Verify<ClassType>();
        Details::InterfaceTraits<I6>::template Verify<ClassType>();
        Details::InterfaceTraits<I7>::template Verify<ClassType>();
        Details::InterfaceTraits<I8>::template Verify<ClassType>();
        Details::InterfaceTraits<I9>::template Verify<ClassType>();

        Details::VerifyInheritanceHelper<BaseType, I1>::Verify();
        Details::VerifyInheritanceHelper<BaseType, I2>::Verify();
        Details::VerifyInheritanceHelper<BaseType, I3>::Verify();
        Details::VerifyInheritanceHelper<BaseType, I4>::Verify();
        Details::VerifyInheritanceHelper<BaseType, I5>::Verify();
        Details::VerifyInheritanceHelper<BaseType, I6>::Verify();
        Details::VerifyInheritanceHelper<BaseType, I7>::Verify();
        Details::VerifyInheritanceHelper<BaseType, I8>::Verify();
        Details::VerifyInheritanceHelper<BaseType, I9>::Verify();
    }

    HRESULT CanCastTo(REFIID riid, void** ppv) noexcept
    {
        BaseType* ptr = static_cast<BaseType*>(static_cast<DerivedType*>(this));

        return (
            Details::InterfaceTraits<I1>::CanCastTo(ptr, riid, ppv) ||
            Details::InterfaceTraits<I2>::CanCastTo(ptr, riid, ppv) ||
            Details::InterfaceTraits<I3>::CanCastTo(ptr, riid, ppv) ||
            Details::InterfaceTraits<I4>::CanCastTo(ptr, riid, ppv) ||
            Details::InterfaceTraits<I5>::CanCastTo(ptr, riid, ppv) ||
            Details::InterfaceTraits<I6>::CanCastTo(ptr, riid, ppv) ||
            Details::InterfaceTraits<I7>::CanCastTo(ptr, riid, ppv) ||
            Details::InterfaceTraits<I8>::CanCastTo(ptr, riid, ppv) ||
            Details::InterfaceTraits<I9>::CanCastTo(ptr, riid, ppv)) ? S_OK : E_NOINTERFACE;
    }

    // It's not possible to cast to IUnknown when Base interface inherit more interfaces
    // The RuntimeClass is taking always the first interface as IUnknown thus it's required to
    // list IInspectable or IUnknown class before MixIn<Derived, MixInType> parameter, such as:
    // struct MyRuntimeClass : RuntimeClass<IInspectable, ChainInterfaces<MixIn<MyRuntimeClass,MyIndependentImplementation>, IFoo, IBar>, MyIndependentImplementation  {}
    IUnknown* CastToUnknown() noexcept = delete;

    static const unsigned long IidCount =
        Details::InterfaceTraits<I1>::IidCount +
        Details::InterfaceTraits<I2>::IidCount +
        Details::InterfaceTraits<I3>::IidCount +
        Details::InterfaceTraits<I4>::IidCount +
        Details::InterfaceTraits<I5>::IidCount +
        Details::InterfaceTraits<I6>::IidCount +
        Details::InterfaceTraits<I7>::IidCount +
        Details::InterfaceTraits<I8>::IidCount +
        Details::InterfaceTraits<I9>::IidCount;

    static void FillArrayWithIid(unsigned long* index, IID* iids) noexcept
    {
        Details::InterfaceTraits<I1>::FillArrayWithIid(index, iids);
        Details::InterfaceTraits<I2>::FillArrayWithIid(index, iids);
        Details::InterfaceTraits<I3>::FillArrayWithIid(index, iids);
        Details::InterfaceTraits<I4>::FillArrayWithIid(index, iids);
        Details::InterfaceTraits<I5>::FillArrayWithIid(index, iids);
        Details::InterfaceTraits<I6>::FillArrayWithIid(index, iids);
        Details::InterfaceTraits<I7>::FillArrayWithIid(index, iids);
        Details::InterfaceTraits<I8>::FillArrayWithIid(index, iids);
        Details::InterfaceTraits<I9>::FillArrayWithIid(index, iids);
    }
};

namespace Details
{

#pragma region Implements helper templates

// Helper template used by Implements. This template traverses a list of interfaces and adds them as base class and information
// to enable QI. doStrictCheck is typically false only for the first interface, allowing IInspectable to be explicitly specified
// only as the first interface.
template <typename RuntimeClassFlagsT, bool doStrictCheck, typename ...TInterfaces>
struct ImplementsHelper;

template <typename T>
struct ImplementsMarker
{};

template <typename I0, bool isImplements>
struct MarkImplements;

template <typename I0>
struct MarkImplements<I0, false>
{
    typedef I0 Type;
};

template <typename I0>
struct MarkImplements<I0, true>
{
    typedef ImplementsMarker<I0> Type;
};

template <typename I0>
struct MarkImplements<CloakedIid<I0>, true>
{
    // Cloaked Implements type will be handled in the nested processing.
    // Applying the ImplementsMarker too early will bypass Cloaked behavior.
    typedef CloakedIid<I0> Type;
};

template <typename DerivedType, typename BaseType, bool hasImplements>
struct MarkImplements<MixIn<DerivedType, BaseType, hasImplements>, true>
{
    // Implements type in mix-ins will be handled in the nested processing.
    typedef MixIn<DerivedType, BaseType, hasImplements> Type;
};

// AdjustImplements pre-processes the type list for more efficient builds.
template <typename RuntimeClassFlagsT, bool doStrictCheck, typename ...Bases>
struct AdjustImplements;

template <typename RuntimeClassFlagsT, bool doStrictCheck, typename I0, typename ...Bases>
struct AdjustImplements<RuntimeClassFlagsT, doStrictCheck, I0, Bases...>
{
    typedef ImplementsHelper<RuntimeClassFlagsT, doStrictCheck, typename MarkImplements<I0, __is_base_of(ImplementsBase, I0)>::Type, Bases...> Type;
};

// Use AdjustImplements to remove instances of "Details::Nil" from the type list.
template <typename RuntimeClassFlagsT, bool doStrictCheck, typename ...Bases>
struct AdjustImplements<RuntimeClassFlagsT, doStrictCheck, Details::Nil, Bases...>
{
    typedef typename AdjustImplements<RuntimeClassFlagsT, doStrictCheck, Bases...>::Type Type;
};

template <typename RuntimeClassFlagsT, bool doStrictCheck>
struct AdjustImplements<RuntimeClassFlagsT, doStrictCheck>
{
    typedef ImplementsHelper<RuntimeClassFlagsT, doStrictCheck> Type;
};

template <unsigned int RuntimeClassTypeT> class RuntimeClassBaseT;

// Specialization handles unadorned interfaces
template <typename RuntimeClassFlagsT, bool doStrictCheck, typename I0, typename ...TInterfaces>
struct ImplementsHelper<RuntimeClassFlagsT, doStrictCheck, I0, TInterfaces...> :
    I0,
    AdjustImplements<RuntimeClassFlagsT, true, TInterfaces...>::Type
{
    template <typename OtherRuntimeClassFlagsT, bool OtherDoStrictCheck, typename ...TOtherInterfaces> friend struct ImplementsHelper;
    template <unsigned int RuntimeClassTypeT> friend class RuntimeClassBaseT;

protected:

    HRESULT CanCastTo(REFIID riid, void** ppv, bool* pRefDelegated = nullptr) noexcept
    {
        VerifyInterfaceHelper<RuntimeClassFlagsT::value & WinRtClassicComMix, I0, doStrictCheck>::Verify();
        // Prefer InlineIsEqualGUID over other forms since it's better perf on 4-byte aligned data, which is almost always the case.
        if (InlineIsEqualGUID(riid, __uuidof(I0)))
        {
            // I0 is the first (primary) base, so it lives at offset 0. A
            // static_cast could be ambiguous (e.g. IInspectable listed first).
            *ppv = reinterpret_cast<I0*>(reinterpret_cast<void*>(this));
            return S_OK;
        }
        return AdjustImplements<RuntimeClassFlagsT, true, TInterfaces...>::Type::CanCastTo(riid, ppv, pRefDelegated);
    }

    IUnknown* CastToUnknown() noexcept
    {
        return reinterpret_cast<I0*>(reinterpret_cast<void*>(this));
    }

    unsigned long GetIidCount() noexcept
    {
        return 1 + AdjustImplements<RuntimeClassFlagsT, true, TInterfaces...>::Type::GetIidCount();
    }

    // FillArrayWithIid
    void FillArrayWithIid(unsigned long* index, IID* iids) noexcept
    {
        *(iids + *index) = __uuidof(I0);
        (*index)++;
        AdjustImplements<RuntimeClassFlagsT, true, TInterfaces...>::Type::FillArrayWithIid(index, iids);
    }
};


// Selector is used to "tag" base interfaces to be used in casting, since a runtime class may indirectly derive from
// the same interface or Implements<> template multiple times
template <typename base, typename disciminator>
struct Selector : public base
{
};

// Specialization handles types that derive from ImplementsHelper (e.g. nested Implements).
template <typename RuntimeClassFlagsT, bool doStrictCheck, typename I0, typename ...TInterfaces>
struct ImplementsHelper<RuntimeClassFlagsT, doStrictCheck, ImplementsMarker<I0>, TInterfaces...> :
    Selector<I0, ImplementsHelper<RuntimeClassFlagsT, doStrictCheck, ImplementsMarker<I0>, TInterfaces...>>,
    Selector<typename AdjustImplements<RuntimeClassFlagsT, true, TInterfaces...>::Type, ImplementsHelper<RuntimeClassFlagsT, doStrictCheck, ImplementsMarker<I0>, TInterfaces...>>
{
    template <typename OtherRuntimeClassFlagsT, bool OtherDoStrictCheck, typename ...TOtherInterfaces> friend struct ImplementsHelper;
    template <unsigned int RuntimeClassTypeT> friend class RuntimeClassBaseT;

protected:
    typedef Selector<I0, ImplementsHelper<RuntimeClassFlagsT, doStrictCheck, ImplementsMarker<I0>, TInterfaces...>> CurrentType;
    typedef Selector<typename AdjustImplements<RuntimeClassFlagsT, true, TInterfaces...>::Type, ImplementsHelper<RuntimeClassFlagsT, doStrictCheck, ImplementsMarker<I0>, TInterfaces...>> BaseType;

    HRESULT CanCastTo(REFIID riid, void** ppv, bool* pRefDelegated = nullptr) noexcept
    {
        VerifyInterfaceHelper<RuntimeClassFlagsT::value & WinRtClassicComMix, I0, doStrictCheck>::Verify();
        HRESULT hr = CurrentType::CanCastTo(riid, ppv);
        if (hr == E_NOINTERFACE)
        {
            hr = BaseType::CanCastTo(riid, ppv, pRefDelegated);
        }
        return hr;
    }

    IUnknown* CastToUnknown() noexcept
    {
        // First in list wins.
        return CurrentType::CastToUnknown();
    }

    unsigned long GetIidCount() noexcept
    {
        return CurrentType::GetIidCount() + BaseType::GetIidCount();
    }

    // FillArrayWithIid
    void FillArrayWithIid(unsigned long* index, IID* iids) noexcept
    {
        CurrentType::FillArrayWithIid(index, iids);
        BaseType::FillArrayWithIid(index, iids);
    }
};

// CloakedIid instance. Since the first "real" interface should be checked against doStrictCheck,
// pass this through unchanged. Two specializations for cloaked prevent the need to use the Selector
// used in the Implements<> case. The same can't be done there because some type ambiguities are unavoidable.
template <typename RuntimeClassFlagsT, bool doStrictCheck, typename I0, typename I1, typename ...TInterfaces>
struct ImplementsHelper<RuntimeClassFlagsT, doStrictCheck, CloakedIid<I0>, I1, TInterfaces...> :
    AdjustImplements<RuntimeClassFlagsT, doStrictCheck, I0>::Type,
    AdjustImplements<RuntimeClassFlagsT, true, I1, TInterfaces...>::Type
{
    template <typename OtherRuntimeClassFlagsT, bool OtherDoStrictCheck, typename ...TOtherInterfaces> friend struct ImplementsHelper;
    template <unsigned int RuntimeClassTypeT> friend class RuntimeClassBaseT;

protected:

    typedef typename AdjustImplements<RuntimeClassFlagsT, doStrictCheck, I0>::Type CurrentType;
    typedef typename AdjustImplements<RuntimeClassFlagsT, true, I1, TInterfaces...>::Type BaseType;

    HRESULT CanCastTo(REFIID riid, void** ppv, bool* pRefDelegated = nullptr) noexcept
    {
        VerifyInterfaceHelper<RuntimeClassFlagsT::value & WinRtClassicComMix, I0, doStrictCheck>::Verify();

        HRESULT hr = CurrentType::CanCastTo(riid, ppv, pRefDelegated);
        if (SUCCEEDED(hr))
        {
            return S_OK;
        }
        return BaseType::CanCastTo(riid, ppv, pRefDelegated);
    }

    IUnknown* CastToUnknown() noexcept
    {
        return CurrentType::CastToUnknown();
    }

    // Don't expose the cloaked IID(s), but continue processing the rest of the interfaces
    unsigned long GetIidCount() noexcept
    {
        return BaseType::GetIidCount();
    }

    void FillArrayWithIid(unsigned long* index, IID* iids) noexcept
    {
        BaseType::FillArrayWithIid(index, iids);
    }
};

template <typename RuntimeClassFlagsT, bool doStrictCheck, typename I0>
struct ImplementsHelper<RuntimeClassFlagsT, doStrictCheck, CloakedIid<I0>> :
    AdjustImplements<RuntimeClassFlagsT, doStrictCheck, I0>::Type
{
    template <typename OtherRuntimeClassFlagsT, bool OtherDoStrictCheck, typename ...TInterfaces> friend struct ImplementsHelper;
    template <unsigned int RuntimeClassTypeT> friend class RuntimeClassBaseT;

protected:

    typedef typename AdjustImplements<RuntimeClassFlagsT, doStrictCheck, I0>::Type CurrentType;

    HRESULT CanCastTo(REFIID riid, void** ppv, bool* pRefDelegated = nullptr) noexcept
    {
        VerifyInterfaceHelper<RuntimeClassFlagsT::value & WinRtClassicComMix, I0, doStrictCheck>::Verify();

        return CurrentType::CanCastTo(riid, ppv, pRefDelegated);
    }

    IUnknown* CastToUnknown() noexcept
    {
        return CurrentType::CastToUnknown();
    }

    // Don't expose the cloaked IID(s), but continue processing the rest of the interfaces
    unsigned long GetIidCount() noexcept
    {
        return 0;
    }

    void FillArrayWithIid(unsigned long* /*index*/, IID* /*iids*/) noexcept
    {
        // no-op
    }
};


// terminal case specialization.
template <typename RuntimeClassFlagsT, bool doStrictCheck>
struct ImplementsHelper<RuntimeClassFlagsT, doStrictCheck>
{
    template <typename OtherRuntimeClassFlagsT, bool OtherDoStrictCheck, typename ...TInterfaces> friend struct ImplementsHelper;
    template <unsigned int RuntimeClassTypeT> friend class RuntimeClassBaseT;

protected:
    HRESULT CanCastTo(REFIID /*riid*/, void** /*ppv*/, bool* /*pRefDelegated*/ = nullptr) noexcept
    {
        return E_NOINTERFACE;
    }

    // IUnknown* CastToUnknown() noexcept; // not defined for terminal case.

    unsigned long GetIidCount() noexcept
    {
        return 0;
    }

    void FillArrayWithIid(unsigned long* /*index*/, IID* /*iids*/) noexcept
    {
    }
};

// Specialization handles chaining interfaces
template <typename RuntimeClassFlagsT, bool doStrictCheck, typename C0, typename C1, typename C2, typename C3, typename C4, typename C5, typename C6, typename C7, typename C8, typename C9, typename ...TInterfaces>
struct ImplementsHelper<RuntimeClassFlagsT, doStrictCheck, ChainInterfaces<C0, C1, C2, C3, C4, C5, C6, C7, C8, C9>, TInterfaces...> :
    ChainInterfaces<C0, C1, C2, C3, C4, C5, C6, C7, C8, C9>,
    AdjustImplements<RuntimeClassFlagsT, true, TInterfaces...>::Type
{
    template <typename OtherRuntimeClassFlagsT, bool OtherDoStrictCheck, typename ...TOtherInterfaces> friend struct ImplementsHelper;
    template <unsigned int RuntimeClassTypeT> friend class RuntimeClassBaseT;

protected:
    typedef ChainInterfaces<C0, C1, C2, C3, C4, C5, C6, C7, C8, C9> ChainType;
    typedef typename AdjustImplements<RuntimeClassFlagsT, true, TInterfaces...>::Type BaseType;

    HRESULT CanCastTo(REFIID riid, void** ppv, bool* pRefDelegated = nullptr) noexcept
    {
        ChainType::template Verify<RuntimeClassFlagsT::value>();

        HRESULT hr = ChainType::CanCastTo(riid, ppv);
        if (FAILED(hr))
        {
            hr = BaseType::CanCastTo(riid, ppv, pRefDelegated);
        }

        return hr;
    }

    IUnknown* CastToUnknown() noexcept
    {
        return ChainType::CastToUnknown();
    }

    unsigned long GetIidCount() noexcept
    {
        return ChainType::IidCount + BaseType::GetIidCount();
    }

    void FillArrayWithIid(unsigned long* index, IID* iids) noexcept
    {
        ChainType::FillArrayWithIid(index, iids);
        BaseType::FillArrayWithIid(index, iids);
    }
};


// Mixin specialization
template <typename RuntimeClassFlagsT, bool doStrictCheck, typename DerivedType, typename BaseMixInType, bool hasImplements, typename ...TInterfaces>
struct ImplementsHelper<RuntimeClassFlagsT, doStrictCheck, MixIn<DerivedType, BaseMixInType, hasImplements>, TInterfaces...> :
    AdjustImplements<RuntimeClassFlagsT, true, TInterfaces...>::Type
{
    static_assert(hasImplements, "Cannot use MixIn to with a class not deriving from \"Implements\"");

    template <typename OtherRuntimeClassFlagsT, bool OtherDoStrictCheck, typename ...TOtherInterfaces> friend struct ImplementsHelper;
    template <unsigned int RuntimeClassTypeT> friend class RuntimeClassBaseT;

protected:
    typedef typename AdjustImplements<RuntimeClassFlagsT, true, TInterfaces...>::Type BaseType;

    HRESULT CanCastTo(REFIID riid, void** ppv, bool* pRefDelegated = nullptr) noexcept
    {
        VerifyInterfaceHelper<RuntimeClassFlagsT::value & WinRtClassicComMix, BaseMixInType, doStrictCheck>::Verify();

        HRESULT hr = static_cast<BaseMixInType*>(static_cast<DerivedType*>(this))->CanCastTo(riid, ppv);
        if (FAILED(hr))
        {
            hr = BaseType::CanCastTo(riid, ppv, pRefDelegated);
        }

        return hr;
    }

    IUnknown* CastToUnknown() noexcept
    {
        return static_cast<BaseMixInType*>(static_cast<DerivedType*>(this))->CastToUnknown();
    }

    unsigned long GetIidCount() noexcept
    {
        return static_cast<BaseMixInType*>(static_cast<DerivedType*>(this))->GetIidCount() +
            BaseType::GetIidCount();
    }

    void FillArrayWithIid(unsigned long* index, IID* iids) noexcept
    {
        static_cast<BaseMixInType*>(static_cast<DerivedType*>(this))->FillArrayWithIid(index, iids);
        BaseType::FillArrayWithIid(index, iids);
    }
};

// Specialization handles inheriting COM objects. ComposableBase must be the last non-nil interface in the list.
// Trailing nil's are allowed for compatibility with some tools that pad out the list.
template <typename I0, typename ...>
struct AreAllNil
{
    static const bool value = false;
};

template <typename ...TInterfaces>
struct AreAllNil<Microsoft::WRL::Details::Nil, TInterfaces...>
{
    static const bool value = AreAllNil<TInterfaces...>::value;
};

template <>
struct AreAllNil<Microsoft::WRL::Details::Nil>
{
    static const bool value = true;
};

template <typename RuntimeClassFlagsT, bool doStrictCheck, typename FactoryInterface, typename ...TInterfaces>
struct ImplementsHelper<RuntimeClassFlagsT, doStrictCheck, ComposableBase<FactoryInterface>, TInterfaces...> :
    ImplementsHelper<RuntimeClassFlagsT, true, ComposableBase<FactoryInterface>>
{
    template <typename OtherRuntimeClassFlagsT, bool OtherDoStrictCheck, typename ...TOtherInterfaces> friend struct ImplementsHelper;
    template <unsigned int RuntimeClassTypeT> friend class RuntimeClassBaseT;

protected:
    typedef ImplementsHelper<RuntimeClassFlagsT, true, ComposableBase<FactoryInterface>> BaseType;

    HRESULT CanCastTo(REFIID riid, void** ppv, bool* pRefDelegated = nullptr) noexcept
    {
        static_assert(AreAllNil<TInterfaces...>::value, "ComposableBase should be the last template parameter to RuntimeClass");
        return BaseType::CanCastTo(riid, ppv, pRefDelegated);
    }

    IUnknown* CastToUnknown() noexcept
    {
        static_assert(AreAllNil<TInterfaces...>::value, "ComposableBase should be the last template parameter to RuntimeClass");
        return BaseType::CastToUnknown();
    }

    unsigned long GetIidCount() noexcept
    {
        static_assert(AreAllNil<TInterfaces...>::value, "ComposableBase should be the last template parameter to RuntimeClass");
        return BaseType::GetIidCount();
    }

    void FillArrayWithIid(unsigned long* index, IID* iids) noexcept
    {
        static_assert(AreAllNil<TInterfaces...>::value, "ComposableBase should be the last template parameter to RuntimeClass");
        BaseType::FillArrayWithIid(index, iids);
    }
};

template <typename RuntimeClassFlagsT, bool doStrictCheck, typename FactoryInterface>
struct ImplementsHelper<RuntimeClassFlagsT, doStrictCheck, ComposableBase<FactoryInterface>>
{
    template <typename OtherRuntimeClassFlagsT, bool OtherDoStrictCheck, typename ...TInterfaces> friend struct ImplementsHelper;
    template <unsigned int RuntimeClassTypeT> friend class RuntimeClassBaseT;

protected:
    HRESULT CanCastTo(REFIID riid, void** ppv, bool* pRefDelegated) noexcept
    {
        *pRefDelegated = true;
        return composableBase_.CopyTo(riid, ppv);
    }

    IUnknown* CastToUnknown() noexcept
    {
        return nullptr;
    }

    unsigned long GetIidCount() noexcept
    {
        return iidCount_;
    }

    void FillArrayWithIid(unsigned long* index, IID* iids) noexcept
    {
        for (unsigned long i = 0; i < iidCount_; i++)
        {
            *(iids + *index) = *(iidsCached_ + i);
            (*index)++;
        }
    }

    ImplementsHelper() noexcept : iidsCached_(nullptr), iidCount_(0)
    {
    }

    ~ImplementsHelper() noexcept
    {
        ::CoTaskMemFree(iidsCached_);
        iidsCached_ = nullptr;
        iidCount_ = 0;
    }

public:
    HRESULT SetComposableBasePointers(IInspectable* base, FactoryInterface* baseFactory = nullptr) noexcept
    {
        if (composableBase_ != nullptr)
        {
            ErrorHelper<RuntimeClassFlagsT::value & InhibitRoOriginateError>::OriginateError(E_UNEXPECTED, nullptr);
            return E_UNEXPECTED;
        }

        HRESULT hr = base->GetIids(&iidCount_, &iidsCached_);
        if (SUCCEEDED(hr))
        {
            composableBase_ = base;
            composableBaseFactory_ = baseFactory;
        }
        return hr;
    }

    ComPtr<IInspectable> GetComposableBase() noexcept
    {
        return composableBase_;
    }

    ComPtr<FactoryInterface> GetComposableBaseFactory() noexcept
    {
        return composableBaseFactory_;
    }

private:
    ComPtr<IInspectable> composableBase_;
    ComPtr<FactoryInterface> composableBaseFactory_;
    IID* iidsCached_;
    ULONG iidCount_;
};

#pragma endregion // Implements helper templates

} // namespace Details

// Implements - template implementing QI using the information provided through its template parameters
// Each template parameter has to be one of the following:
// * COM Interface
// * A class that implements one or more COM interfaces
// * ChainInterfaces template
template <typename I0, typename ...TInterfaces>
struct Implements :
    Details::AdjustImplements<RuntimeClassFlags<WinRt>, true, I0, TInterfaces...>::Type,
    Details::ImplementsBase
{
public:
    typedef RuntimeClassFlags<WinRt> ClassFlags;
    typedef I0 FirstInterface;
protected:
    typedef typename Details::AdjustImplements<RuntimeClassFlags<WinRt>, true, I0, TInterfaces...>::Type BaseType;
    template <typename RuntimeClassFlagsT, bool doStrictCheck, typename ...TOtherInterfaces> friend struct Details::ImplementsHelper;
    template <unsigned int RuntimeClassTypeT> friend class Details::RuntimeClassBaseT;

    HRESULT CanCastTo(REFIID riid, void** ppv) noexcept
    {
        return BaseType::CanCastTo(riid, ppv);
    }

    IUnknown* CastToUnknown() noexcept
    {
        return BaseType::CastToUnknown();
    }

    unsigned long GetIidCount() noexcept
    {
        return BaseType::GetIidCount();
    }

    void FillArrayWithIid(unsigned long* index, IID* iids) noexcept
    {
        BaseType::FillArrayWithIid(index, iids);
    }
};

// Note: 'flags' must be 'unsigned int' (same as RuntimeClassFlags), otherwise
// the partial specialization never matches on conforming compilers.
template <unsigned int flags, typename I0, typename ...TInterfaces>
struct Implements<RuntimeClassFlags<flags>, I0, TInterfaces...> :
    Details::AdjustImplements<RuntimeClassFlags<flags>, true, I0, TInterfaces...>::Type,
    Details::ImplementsBase
{
public:
    typedef RuntimeClassFlags<flags> ClassFlags;
    typedef I0 FirstInterface;
protected:

    typedef typename Details::AdjustImplements<RuntimeClassFlags<flags>, true, I0, TInterfaces...>::Type BaseType;
    template <typename RuntimeClassFlagsT, bool doStrictCheck, typename ...TOtherInterfaces> friend struct Details::ImplementsHelper;
    template <unsigned int RuntimeClassTypeT> friend class Details::RuntimeClassBaseT;

    HRESULT CanCastTo(REFIID riid, void** ppv) noexcept
    {
        return BaseType::CanCastTo(riid, ppv);
    }

    IUnknown* CastToUnknown() noexcept
    {
        return BaseType::CastToUnknown();
    }

    unsigned long GetIidCount() noexcept
    {
        return BaseType::GetIidCount();
    }

    void FillArrayWithIid(unsigned long* index, IID* iids) noexcept
    {
        BaseType::FillArrayWithIid(index, iids);
    }
};

class FtmBase :
    public Implements<
        ::Microsoft::WRL::RuntimeClassFlags<WinRtClassicComMix>,
        ::Microsoft::WRL::CloakedIid< ::IMarshal> >,
    // Inheriting from FtmBaseMarker allows using FtmBase on classes configured with RuntimeClassFlags<WinRt> (Default configuration)
    private ::Microsoft::WRL::Details::FtmBaseMarker
{
    using Super = Implements<
      ::Microsoft::WRL::RuntimeClassFlags<WinRtClassicComMix>,
      ::Microsoft::WRL::CloakedIid< ::IMarshal> >;
protected:
    template <typename RuntimeClassFlagsT, bool doStrictCheck, typename ...TInterfaces> friend struct Details::ImplementsHelper;

    HRESULT CanCastTo(REFIID riid, void** ppv) noexcept
    {
        // Prefer InlineIsEqualGUID over other forms since it's better perf on 4-byte aligned data, which is almost always the case.
        if (InlineIsEqualGUID(riid, __uuidof(::IAgileObject)))
        {
            *ppv = Super::CastToUnknown();
            return S_OK;
        }

        return Super::CanCastTo(riid, ppv);
    }

public:
    FtmBase() noexcept
    {
        ComPtr<IUnknown> unknown;
        if (SUCCEEDED(::CoCreateFreeThreadedMarshaler(nullptr, unknown.GetAddressOf())))
        {
            unknown.As(&marshaller_);
        }
    }

    // IMarshal Methods
    STDMETHOD(GetUnmarshalClass)(REFIID riid,
                                 void* pv,
                                 DWORD dwDestContext,
                                 void* pvDestContext,
                                 DWORD mshlflags,
                                 CLSID* pCid) override
    {
        if (marshaller_)
        {
            return marshaller_->GetUnmarshalClass(riid, pv, dwDestContext, pvDestContext, mshlflags, pCid);
        }
        return E_OUTOFMEMORY;
    }

    STDMETHOD(GetMarshalSizeMax)(REFIID riid, void* pv, DWORD dwDestContext,
                                 void* pvDestContext, DWORD mshlflags, DWORD* pSize) override
    {
        if (marshaller_)
        {
            return marshaller_->GetMarshalSizeMax(riid, pv, dwDestContext, pvDestContext, mshlflags, pSize);
        }
        return E_OUTOFMEMORY;
    }

    STDMETHOD(MarshalInterface)(IStream* pStm, REFIID riid, void* pv, DWORD dwDestContext,
                                void* pvDestContext, DWORD mshlflags) override
    {
        if (marshaller_)
        {
            return marshaller_->MarshalInterface(pStm, riid, pv, dwDestContext, pvDestContext, mshlflags);
        }
        return E_OUTOFMEMORY;
    }

    STDMETHOD(UnmarshalInterface)(IStream* pStm, REFIID riid, void** ppv) override
    {
        if (marshaller_)
        {
            return marshaller_->UnmarshalInterface(pStm, riid, ppv);
        }
        return E_OUTOFMEMORY;
    }

    STDMETHOD(ReleaseMarshalData)(IStream* pStm) override
    {
        if (marshaller_)
        {
            return marshaller_->ReleaseMarshalData(pStm);
        }
        return E_OUTOFMEMORY;
    }

    STDMETHOD(DisconnectObject)(DWORD dwReserved) override
    {
        if (marshaller_)
        {
            return marshaller_->DisconnectObject(dwReserved);
        }
        return E_OUTOFMEMORY;
    }

    static HRESULT CreateGlobalInterfaceTable(IGlobalInterfaceTable** git) noexcept
    {
        *git = nullptr;
        return ::CoCreateInstance(CLSID_StdGlobalInterfaceTable,
            nullptr,
            CLSCTX_INPROC_SERVER,
            __uuidof(IGlobalInterfaceTable),
            reinterpret_cast<void**>(git));
    }

    ::Microsoft::WRL::ComPtr<IMarshal> marshaller_;  // Holds a reference to the free threaded marshaler
};

namespace Details
{

// Reference count primitives. The SDK uses the NoFence/Release variants on
// ARM; full barrier interlocked operations are correct on every architecture.
#define UnknownIncrementReference InterlockedIncrement
#define UnknownDecrementReference InterlockedDecrement
#define UnknownBarrierAfterInterlock() MemoryBarrier()
#define UnknownInterlockedCompareExchangePointer InterlockedCompareExchangePointer
#define UnknownInterlockedCompareExchangePointerForIncrement InterlockedCompareExchangePointer
#define UnknownInterlockedCompareExchangePointerForRelease InterlockedCompareExchangePointer
#define UnknownInterlockedCompareExchangeForIncrement InterlockedCompareExchange
#define UnknownInterlockedCompareExchangeForRelease InterlockedCompareExchange

// Since variadic templates can't have a parameter pack after default arguments, provide a convenient helper for defaults.
#define DETAILS_RTCLASS_FLAGS_ARGUMENTS(RuntimeClassFlagsT) \
    RuntimeClassFlagsT, \
    (RuntimeClassFlagsT::value & InhibitWeakReference) == 0, \
    (RuntimeClassFlagsT::value & WinRt) == WinRt, \
    __WRL_IMPLEMENTS_FTM_BASE__(RuntimeClassFlagsT::value) \

template <class RuntimeClassFlagsT, bool implementsWeakReferenceSource, bool implementsInspectable, bool implementsFtmBase, typename ...TInterfaces>
class RuntimeClassImpl;

class WeakReferenceImpl;

// Reference counting functions that check overflow. If overflow is detected, ref count value will stop at LONG_MAX, and the object being
// reference-counted will be leaked.
inline unsigned long SafeUnknownIncrementReference(long volatile& refcount) noexcept
{
    long oldValue = refcount;
    while (oldValue != LONG_MAX && (UnknownInterlockedCompareExchangeForIncrement(&refcount, oldValue + 1, oldValue) != oldValue))
    {
        oldValue = refcount;
    }

    if (oldValue != LONG_MAX)
    {
        return static_cast<unsigned long>(oldValue + 1);
    }
    else
    {
        return LONG_MAX;
    }
}

inline unsigned long SafeUnknownDecrementReference(long volatile& refcount) noexcept
{
    long oldValue = refcount;
    while (oldValue != LONG_MAX && (UnknownInterlockedCompareExchangeForRelease(&refcount, oldValue - 1, oldValue) != oldValue))
    {
        oldValue = refcount;
    }

    return static_cast<unsigned long>(oldValue - 1);
}

template <class RuntimeClassFlagsT, bool implementsWeakReferenceSource, bool implementsFtmBase, typename ...TInterfaces>
class RuntimeClassImpl<RuntimeClassFlagsT, implementsWeakReferenceSource, false, implementsFtmBase, TInterfaces...> :
    public Details::AdjustImplements<RuntimeClassFlagsT, false, TInterfaces...>::Type,
    public RuntimeClassBaseT<RuntimeClassFlagsT::value>,
    protected RuntimeClassFlags<InhibitWeakReference>,
    public DontUseNewUseMake
{
public:
    typedef RuntimeClassFlagsT ClassFlags;

    STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject)
    {
        return Super::AsIID(this, riid, ppvObject);
    }

    STDMETHOD_(ULONG, AddRef)()
    {
        return InternalAddRef();
    }

    STDMETHOD_(ULONG, Release)()
    {
        ULONG ref = InternalRelease();
        if (ref == 0)
        {
            delete this;

            auto modulePtr = ::Microsoft::WRL::GetModuleBase();
            if (modulePtr != nullptr)
            {
                modulePtr->DecrementObjectCount();
            }
        }

        return ref;
    }

protected:
    using Super = RuntimeClassBaseT<RuntimeClassFlagsT::value>;

    RuntimeClassImpl() noexcept : refcount_(1)
    {
    }

    virtual ~RuntimeClassImpl() noexcept
    {
        // Set refcount_ to -(LONG_MAX/2) to protect destruction and
        // also catch mismatched Release in debug builds
        refcount_ = -(LONG_MAX/2);
    }

    unsigned long InternalAddRef() noexcept
    {
        return SafeUnknownIncrementReference(refcount_);
    }

    unsigned long InternalRelease() noexcept
    {
        // A release fence is required to ensure all guarded memory accesses are
        // complete before any thread can begin destroying the object.
        unsigned long newValue = SafeUnknownDecrementReference(refcount_);
        if (newValue == 0)
        {
            // An acquire fence is required before object destruction to ensure
            // that the destructor cannot observe values changing on other threads.
            UnknownBarrierAfterInterlock();
        }
        return newValue;
    }

    unsigned long GetRefCount() const noexcept
    {
        return static_cast<unsigned long>(refcount_);
    }

    friend class WeakReferenceImpl;

private:
    volatile long refcount_;
};

template <typename I, bool isImplementsBased = std::is_base_of<ImplementsBase, I>::value>
struct HasIInspectable;

template <typename I>
struct HasIInspectable<I, false>
{
    static const bool isIInspectable = __is_base_of(IInspectable, I);
};

template <typename I>
struct HasIInspectable<I, true>
{
    static const bool isIInspectable = HasIInspectable<typename I::FirstInterface>::isIInspectable;
};

#ifdef __WRL_STRICT__
template <typename I0, bool isIInspectable = true>
#else
template <typename I0, bool isIInspectable = HasIInspectable<I0>::isIInspectable>
#endif
struct IInspectableInjector;

template <typename I0>
struct IInspectableInjector<I0, true>
{
    typedef Details::Nil InspectableIfNeeded;
};

template <typename I0>
struct IInspectableInjector<I0, false>
{
    typedef IInspectable InspectableIfNeeded;
};

// Implements IInspectable in ILst
template <class RuntimeClassFlagsT, typename I0, typename ...TInterfaces>
class RuntimeClassImpl<RuntimeClassFlagsT, false, true, false, I0, TInterfaces...> :
    public Details::AdjustImplements<RuntimeClassFlagsT, false, typename IInspectableInjector<I0>::InspectableIfNeeded, I0, TInterfaces...>::Type,
    public RuntimeClassBaseT<RuntimeClassFlagsT::value>,
    protected RuntimeClassFlags<InhibitWeakReference>,
    public DontUseNewUseMake
{
public:
    typedef RuntimeClassFlagsT ClassFlags;

    STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject)
    {
        return Super::AsIID(this, riid, ppvObject);
    }

    STDMETHOD_(ULONG, AddRef)()
    {
        return InternalAddRef();
    }

    STDMETHOD_(ULONG, Release)()
    {
        ULONG ref = InternalRelease();
        if (ref == 0)
        {
            delete this;

            auto modulePtr = ::Microsoft::WRL::GetModuleBase();
            if (modulePtr != nullptr)
            {
                modulePtr->DecrementObjectCount();
            }
        }

        return ref;
    }

    // IInspectable methods
    STDMETHOD(GetIids)(ULONG* iidCount, IID** iids)
    {
        return Super::GetImplementedIIDS(this, iidCount, iids);
    }

#if !defined(__WRL_STRICT__) || !defined(__WRL_FORCE_INSPECTABLE_CLASS_MACRO__)
    STDMETHOD(GetRuntimeClassName)(HSTRING* runtimeClassName)
    {
        *runtimeClassName = nullptr;

        __WRL_ASSERT__(false && "Use InspectableClass macro to set runtime class name and trust level.");

        ErrorHelper<RuntimeClassFlagsT::value & InhibitRoOriginateError>::OriginateError(E_NOTIMPL, nullptr);
        return E_NOTIMPL;
    }

    STDMETHOD(GetTrustLevel)(::TrustLevel*)
    {
        __WRL_ASSERT__(false && "Use InspectableClass macro to set runtime class name and trust level.");

        ErrorHelper<RuntimeClassFlagsT::value & InhibitRoOriginateError>::OriginateError(E_NOTIMPL, nullptr);
        return E_NOTIMPL;
    }
#endif // !defined(__WRL_STRICT__) || !defined(__WRL_FORCE_INSPECTABLE_CLASS_MACRO__)

protected:
    using Super = RuntimeClassBaseT<RuntimeClassFlagsT::value>;

    RuntimeClassImpl() noexcept : refcount_(1)
    {
    }

    virtual ~RuntimeClassImpl() noexcept
    {
        // Set refcount_ to -(LONG_MAX/2) to protect destruction and
        // also catch mismatched Release in debug builds
        refcount_ = -(LONG_MAX/2);
    }

    unsigned long InternalAddRef() noexcept
    {
        return SafeUnknownIncrementReference(refcount_);
    }

    unsigned long InternalRelease() noexcept
    {
        // A release fence is required to ensure all guarded memory accesses are
        // complete before any thread can begin destroying the object.
        unsigned long newValue = SafeUnknownDecrementReference(refcount_);
        if (newValue == 0)
        {
            // An acquire fence is required before object destruction to ensure
            // that the destructor cannot observe values changing on other threads.
            UnknownBarrierAfterInterlock();
        }
        return newValue;
    }

    unsigned long GetRefCount() const noexcept
    {
        return static_cast<unsigned long>(refcount_);
    }
private:
    volatile long refcount_;
};

class StrongReference
{
public:
    StrongReference(long refCount = 1) noexcept : strongRefCount_(refCount) {}

    ~StrongReference() noexcept
    {
        // Set refcount_ to -(LONG_MAX/2) to protect destruction and
        // also catch mismatched Release in debug builds
        strongRefCount_ = -(LONG_MAX / 2);
    }

    unsigned long IncrementStrongReference() noexcept
    {
        return SafeUnknownIncrementReference(strongRefCount_);
    }

    unsigned long DecrementStrongReference() noexcept
    {
        // A release fence is required to ensure all guarded memory accesses are
        // complete before any thread can begin destroying the object.
        unsigned long newValue = SafeUnknownDecrementReference(strongRefCount_);
        if (newValue == 0)
        {
            // An acquire fence is required before object destruction to ensure
            // that the destructor cannot observe values changing on other threads.
            UnknownBarrierAfterInterlock();
        }
        return newValue;
    }

    unsigned long GetStrongReferenceCount() noexcept
    {
        return static_cast<unsigned long>(strongRefCount_);
    }

    void SetStrongReference(unsigned long value) noexcept
    {
        strongRefCount_ = static_cast<long>(value);
    }

    long strongRefCount_;
};

// To support storing, encoding and decoding reference-count/pointers regardless of the target platform.
// In a RuntimeClass, the refCount_ member can mean either
//    1. actual reference count
//    2. pointer to the weak reference object which holds the strong count
// The member
//    1. If it is a count, the most significant bit will be OFF
//    2. If it is an encoded pointer to the weak reference, the most significant bit will be turned ON
// To test which mode it is
//    1. Test for negative
//    2. If it is, it is an encoded pointer to the weak reference
//    3. If it is not, it is the actual reference count
// To yield the encoded pointer
//    1. Test the value for negative
//    2. If it is, shift the value to the left and cast it to a WeakReferenceImpl*
//
const UINT_PTR EncodeWeakReferencePointerFlag = static_cast<UINT_PTR>(1) << ((sizeof(UINT_PTR) * 8) - 1);

union ReferenceCountOrWeakReferencePointer
{
    // Represents the count when it is a count (in practice only the least significant 4 bytes)
    UINT_PTR refCount;
    // Pointer size, *signed* to help with ease of casting and such
    INT_PTR rawValue;
    // The hint that this could also be a pointer
    void* ifHighBitIsSetThenShiftLeftToYieldPointerToWeakReference;
};

// Helper methods to test, decode and decode the different representations of ReferenceCountOrWeakReferencePointer
inline bool IsValueAPointerToWeakReference(INT_PTR value)
{
    return value < 0;
}

inline INT_PTR EncodeWeakReferencePointer(Microsoft::WRL::Details::WeakReferenceImpl* value);
inline Microsoft::WRL::Details::WeakReferenceImpl* DecodeWeakReferencePointer(INT_PTR value);

// Reads a pointer sized value exactly once (the compiler must not re-fetch it).
template <typename T>
inline T ReadValueFromPointerNoFence(const volatile T* value)
{
    static_assert(sizeof(T) == sizeof(ULONG_PTR), "ReadValueFromPointerNoFence expects a pointer sized value");
    union
    {
        ULONG_PTR raw;
        T typed;
    } result;
    result.raw = *reinterpret_cast<const volatile ULONG_PTR*>(value);
    return result.typed;
}

inline WeakReferenceImpl* CreateWeakReference(IUnknown*);

// Implementation of activatable class that implements IWeakReferenceSource
// and delegates reference counting to WeakReferenceImpl object
template <class RuntimeClassFlagsT, typename I0, typename ...TInterfaces>
class RuntimeClassImpl<RuntimeClassFlagsT, true, true, false, I0, TInterfaces...> :
    public Details::AdjustImplements<RuntimeClassFlagsT, false, typename IInspectableInjector<I0>::InspectableIfNeeded, I0, IWeakReferenceSource, TInterfaces...>::Type,
    public RuntimeClassBaseT<RuntimeClassFlagsT::value>,
    public DontUseNewUseMake
{
public:
    typedef RuntimeClassFlagsT ClassFlags;

    RuntimeClassImpl() noexcept
    {
        refCount_.rawValue = 1;
    }

    STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject)
    {
        return Super::AsIID(this, riid, ppvObject);
    }

    STDMETHOD_(ULONG, AddRef)()
    {
        return InternalAddRef();
    }

    STDMETHOD_(ULONG, Release)()
    {
        ULONG ref = InternalRelease();
        if (ref == 0)
        {
            delete this;

            auto modulePtr = ::Microsoft::WRL::GetModuleBase();
            if (modulePtr != nullptr)
            {
                modulePtr->DecrementObjectCount();
            }
        }

        return ref;
    }

    // IInspectable methods
    STDMETHOD(GetIids)(ULONG* iidCount, IID** iids)
    {
        return Super::GetImplementedIIDS(this, iidCount, iids);
    }

#if !defined(__WRL_STRICT__) || !defined(__WRL_FORCE_INSPECTABLE_CLASS_MACRO__)
    STDMETHOD(GetRuntimeClassName)(HSTRING* runtimeClassName)
    {
        *runtimeClassName = nullptr;

        __WRL_ASSERT__(false && "Use InspectableClass macro to set runtime class name and trust level.");

        ErrorHelper<RuntimeClassFlagsT::value & InhibitRoOriginateError>::OriginateError(E_NOTIMPL, nullptr);
        return E_NOTIMPL;
    }

    STDMETHOD(GetTrustLevel)(::TrustLevel*)
    {
        __WRL_ASSERT__(false && "Use InspectableClass macro to set runtime class name and trust level.");

        ErrorHelper<RuntimeClassFlagsT::value & InhibitRoOriginateError>::OriginateError(E_NOTIMPL, nullptr);
        return E_NOTIMPL;
    }
#endif // !defined(__WRL_STRICT__) || !defined(__WRL_FORCE_INSPECTABLE_CLASS_MACRO__)

    STDMETHOD(GetWeakReference)(IWeakReference** weakReference);

    virtual ~RuntimeClassImpl() noexcept;

protected:
    template <unsigned int RuntimeClassTypeT> friend class Details::RuntimeClassBaseT;
    using ImplementsHelperT = typename Details::AdjustImplements<RuntimeClassFlagsT, false, typename IInspectableInjector<I0>::InspectableIfNeeded, I0, IWeakReferenceSource, TInterfaces...>::Type;
    using Super = RuntimeClassBaseT<RuntimeClassFlagsT::value>;

    unsigned long InternalAddRef() noexcept;

    unsigned long InternalRelease() noexcept;

    unsigned long GetRefCount() const noexcept;

    friend class WeakReferenceImpl;

#ifdef __WRL_UNITTEST__
protected:
#else
private:
#endif
    ReferenceCountOrWeakReferencePointer refCount_;
};

inline INT_PTR EncodeWeakReferencePointer(Microsoft::WRL::Details::WeakReferenceImpl* value)
{
    return static_cast<INT_PTR>((reinterpret_cast<UINT_PTR>(value) >> 1) | EncodeWeakReferencePointerFlag);
}

inline Microsoft::WRL::Details::WeakReferenceImpl* DecodeWeakReferencePointer(INT_PTR value)
{
    return reinterpret_cast<Microsoft::WRL::Details::WeakReferenceImpl*>(static_cast<UINT_PTR>(value) << 1);
}

template <class RuntimeClassFlagsT, typename I0, typename ...TInterfaces>
class RuntimeClassImpl<RuntimeClassFlagsT, false, true, true, I0, TInterfaces...> :
    public RuntimeClassImpl<RuntimeClassFlagsT, false, true, false, I0, TInterfaces...>
{
};

template <class RuntimeClassFlagsT, typename I0, typename ...TInterfaces>
class RuntimeClassImpl<RuntimeClassFlagsT, true, true, true, I0, TInterfaces...> :
    public RuntimeClassImpl<RuntimeClassFlagsT, true, true, false, I0, FtmBase, TInterfaces...>
{
};

// To minimize breaks with code written against WRL before variadic support was added, this form is maintained.
template <typename ...TInterfaces>
struct InterfaceListHelper
{
    typedef InterfaceListHelper<TInterfaces...> TypeT;
};

template <
    typename ILst,
    class RuntimeClassFlagsT,
    bool implementsWeakReferenceSource = (RuntimeClassFlagsT::value & InhibitWeakReference) == 0,
    bool implementsInspectable = (RuntimeClassFlagsT::value & WinRt) == WinRt,
    bool implementsFtmBase = __WRL_IMPLEMENTS_FTM_BASE__(RuntimeClassFlagsT::value)
>
class RuntimeClass;

template <
    typename RuntimeClassFlagsT,
    bool implementsWeakReferenceSource,
    bool implementsInspectable,
    bool implementsFtmBase,
    typename ...TInterfaces
>
class RuntimeClass<InterfaceListHelper<TInterfaces...>, RuntimeClassFlagsT, implementsWeakReferenceSource, implementsInspectable, implementsFtmBase> :
    public RuntimeClassImpl<RuntimeClassFlagsT, implementsWeakReferenceSource, implementsInspectable, implementsFtmBase, TInterfaces...>
{
protected:
    HRESULT CustomQueryInterface(REFIID /*riid*/, void** /*ppvObject*/, bool* handled)
    {
        *handled = false;
        return S_OK;
    }
};

} // namespace Details

// The RuntimeClass IUnknown methods
// It inherits from Details::RuntimeClass that provides helper methods for reference counting and
// collecting IIDs
template <typename ...TInterfaces>
class RuntimeClass :
    public Details::RuntimeClassImpl<DETAILS_RTCLASS_FLAGS_ARGUMENTS(RuntimeClassFlags<WinRt>), TInterfaces...>
{
    RuntimeClass(const RuntimeClass&) = delete;
    RuntimeClass& operator=(const RuntimeClass&) = delete;
protected:
    HRESULT CustomQueryInterface(REFIID /*riid*/, void** /*ppvObject*/, bool* handled)
    {
        *handled = false;
        return S_OK;
    }
public:
    RuntimeClass() noexcept
    {
        auto modulePtr = ::Microsoft::WRL::GetModuleBase();
        if (modulePtr != nullptr)
        {
            modulePtr->IncrementObjectCount();
        }
    }
    typedef RuntimeClass RuntimeClassT;
};

template <unsigned int classFlags, typename ...TInterfaces>
class RuntimeClass<RuntimeClassFlags<classFlags>, TInterfaces...> :
    public Details::RuntimeClassImpl<DETAILS_RTCLASS_FLAGS_ARGUMENTS(RuntimeClassFlags<classFlags>), TInterfaces...>
{
    RuntimeClass(const RuntimeClass&) = delete;
    RuntimeClass& operator=(const RuntimeClass&) = delete;
protected:
    HRESULT CustomQueryInterface(REFIID /*riid*/, void** /*ppvObject*/, bool* handled)
    {
        *handled = false;
        return S_OK;
    }
public:
    RuntimeClass() noexcept
    {
        auto modulePtr = ::Microsoft::WRL::GetModuleBase();
        if (modulePtr != nullptr)
        {
            modulePtr->IncrementObjectCount();
        }
    }
    typedef RuntimeClass RuntimeClassT;
};

namespace Details
{
    // Weak reference implementation
    class WeakReferenceImpl final :
        public ::Microsoft::WRL::RuntimeClass<RuntimeClassFlags<ClassicCom>, IWeakReference>,
        public StrongReference
    {
    public:
        WeakReferenceImpl(IUnknown* unk) noexcept : StrongReference(LONG_MAX / 2), unknown_(unk)
        {
            // Set ref count to 2 to avoid unnecessary interlocked increment operation while returning
            // WeakReferenceImpl from GetWeakReference method. One reference is hold by the object the second is hold
            // by the caller of GetWeakReference method.
            refcount_ = 2;
        }

        virtual ~WeakReferenceImpl() noexcept
        {
        }

        STDMETHOD(Resolve)(REFIID riid, IInspectable** ppvObject) override
        {
            *ppvObject = nullptr;

            for (;;)
            {
                long ref = this->strongRefCount_;
                if (ref == 0)
                {
                    return S_OK;
                }

                if (::InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&this->strongRefCount_), ref + 1, ref) == ref)
                {
                    break;
                }
            }

            HRESULT hr = unknown_->QueryInterface(riid, reinterpret_cast<void**>(ppvObject));
            unknown_->Release();
            return hr;
        }

    private:
        IUnknown* unknown_;
    };

template <class RuntimeClassFlagsT, typename I0, typename ...TInterfaces>
RuntimeClassImpl<RuntimeClassFlagsT, true, true, false, I0, TInterfaces...>::~RuntimeClassImpl() noexcept
{
    if (IsValueAPointerToWeakReference(refCount_.rawValue))
    {
        WeakReferenceImpl* weakRef = DecodeWeakReferencePointer(refCount_.rawValue);
        weakRef->Release();
        weakRef = nullptr;
    }
}

template <class RuntimeClassFlagsT, typename I0, typename ...TInterfaces>
unsigned long RuntimeClassImpl<RuntimeClassFlagsT, true, true, false, I0, TInterfaces...>::GetRefCount() const noexcept
{
    ReferenceCountOrWeakReferencePointer currentValue = ReadValueFromPointerNoFence<ReferenceCountOrWeakReferencePointer>(&refCount_);

    if (IsValueAPointerToWeakReference(currentValue.rawValue))
    {
        WeakReferenceImpl* weakRef = DecodeWeakReferencePointer(currentValue.rawValue);
        return weakRef->GetStrongReferenceCount();
    }
    else
    {
        return static_cast<unsigned long>(currentValue.refCount);
    }
}

template <class RuntimeClassFlagsT, typename I0, typename ...TInterfaces>
unsigned long RuntimeClassImpl<RuntimeClassFlagsT, true, true, false, I0, TInterfaces...>::InternalAddRef() noexcept
{
    ReferenceCountOrWeakReferencePointer currentValue = ReadValueFromPointerNoFence<ReferenceCountOrWeakReferencePointer>(&refCount_);

    for (;;)
    {
        if (!IsValueAPointerToWeakReference(currentValue.rawValue))
        {
            if (static_cast<long>(currentValue.refCount) == LONG_MAX)
            {
                return LONG_MAX;
            }

            UINT_PTR updateValue = currentValue.refCount + 1;

            INT_PTR previousValue = reinterpret_cast<INT_PTR>(UnknownInterlockedCompareExchangePointerForIncrement(reinterpret_cast<PVOID*>(&(refCount_.refCount)), reinterpret_cast<PVOID>(updateValue), reinterpret_cast<PVOID>(currentValue.refCount)));
            if (previousValue == currentValue.rawValue)
            {
                return static_cast<unsigned long>(updateValue);
            }

            currentValue.rawValue = previousValue;
        }
        else
        {
            WeakReferenceImpl* weakRef = DecodeWeakReferencePointer(currentValue.rawValue);
            return weakRef->IncrementStrongReference();
        }
    }
}

template <class RuntimeClassFlagsT, typename I0, typename ...TInterfaces>
unsigned long RuntimeClassImpl<RuntimeClassFlagsT, true, true, false, I0, TInterfaces...>::InternalRelease() noexcept
{
    ReferenceCountOrWeakReferencePointer currentValue = ReadValueFromPointerNoFence<ReferenceCountOrWeakReferencePointer>(&refCount_);

    for (;;)
    {
        if (!IsValueAPointerToWeakReference(currentValue.rawValue))
        {
            if (static_cast<long>(currentValue.refCount) == LONG_MAX)
            {
                return LONG_MAX - 1;
            }

            UINT_PTR updateValue = currentValue.refCount - 1;

            INT_PTR previousValue = reinterpret_cast<INT_PTR>(UnknownInterlockedCompareExchangePointerForRelease(reinterpret_cast<PVOID*>(&(refCount_.refCount)), reinterpret_cast<PVOID>(updateValue), reinterpret_cast<PVOID>(currentValue.refCount)));
            if (previousValue == currentValue.rawValue)
            {
                if (updateValue == 0)
                {
                    UnknownBarrierAfterInterlock();
                }
                return static_cast<unsigned long>(updateValue);
            }

            currentValue.rawValue = previousValue;
        }
        else
        {
            WeakReferenceImpl* weakRef = DecodeWeakReferencePointer(currentValue.rawValue);
            return weakRef->DecrementStrongReference();
        }
    }
}

template <class RuntimeClassFlagsT, typename I0, typename ...TInterfaces>
COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE RuntimeClassImpl<RuntimeClassFlagsT, true, true, false, I0, TInterfaces...>::GetWeakReference(IWeakReference** weakReference)
{
    WeakReferenceImpl* weakRef = nullptr;
    INT_PTR encodedWeakRef = 0;
    ReferenceCountOrWeakReferencePointer currentValue = ReadValueFromPointerNoFence<ReferenceCountOrWeakReferencePointer>(&refCount_);

    *weakReference = nullptr;

    if (IsValueAPointerToWeakReference(currentValue.rawValue))
    {
        weakRef = DecodeWeakReferencePointer(currentValue.rawValue);

        weakRef->AddRef();
        *weakReference = weakRef;
        return S_OK;
    }

    // WeakReferenceImpl is created with ref count 2 to avoid interlocked increment
    weakRef = CreateWeakReference(ImplementsHelperT::CastToUnknown());
    if (weakRef == nullptr)
    {
        return E_OUTOFMEMORY;
    }

    encodedWeakRef = EncodeWeakReferencePointer(weakRef);

    for (;;)
    {
        INT_PTR previousValue = 0;

        weakRef->SetStrongReference(static_cast<unsigned long>(currentValue.refCount));

        previousValue = reinterpret_cast<INT_PTR>(UnknownInterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&(this->refCount_.ifHighBitIsSetThenShiftLeftToYieldPointerToWeakReference)), reinterpret_cast<PVOID>(encodedWeakRef), currentValue.ifHighBitIsSetThenShiftLeftToYieldPointerToWeakReference));
        if (previousValue == currentValue.rawValue)
        {
            // No need to call AddRef in this case, WeakReferenceImpl is created with ref count 2 to avoid interlocked increment
            *weakReference = weakRef;
            return S_OK;
        }
        else if (IsValueAPointerToWeakReference(previousValue))
        {
            // Another thread beat this call to create the weak reference.

            delete weakRef;

            weakRef = DecodeWeakReferencePointer(previousValue);
            weakRef->AddRef();
            *weakReference = weakRef;
            return S_OK;
        }

        // Another thread won via an AddRef or Release.
        // Let's try again
        currentValue.rawValue = previousValue;
    }
}

// Memory allocation for object that doesn't support weak references
// It only allocates memory
template <typename T>
class MakeAllocator
{
public:
    MakeAllocator() noexcept : buffer_(nullptr)
    {
    }

    ~MakeAllocator() noexcept
    {
        if (buffer_ != nullptr)
        {
            if constexpr (alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
            {
                ::operator delete(buffer_, static_cast<std::align_val_t>(alignof(T)));
            }
            else
            {
                ::operator delete(buffer_);
            }
        }
    }

    void* Allocate() noexcept
    {
        __WRL_ASSERT__(buffer_ == nullptr);
        // Allocate memory with operator new(size, nothrow) only
        // This will allow developer to override one operator only
        // to enable different memory allocation model
        if constexpr (alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
        {
            return buffer_ = ::operator new(sizeof(T), static_cast<std::align_val_t>(alignof(T)), ::std::nothrow);
        }
        else
        {
            return buffer_ = ::operator new(sizeof(T), ::std::nothrow);
        }
    }

    void Detach() noexcept
    {
        buffer_ = nullptr;
    }
private:
    void* buffer_;
};

} // namespace Details

#pragma region make overloads

namespace Details {

// Make and MakeAndInitialize functions must not be marked as noexcept as the constructor is allowed to throw exceptions.
template <typename T, typename ...TArgs>
ComPtr<T> Make(TArgs&&... args)
{
    static_assert(__is_base_of(Details::RuntimeClassBase, T), "Make can only instantiate types that derive from RuntimeClass");
    ComPtr<T> object;
    Details::MakeAllocator<T> allocator;
    void* buffer = allocator.Allocate();
    if (buffer != nullptr)
    {
        auto ptr = new (buffer) T(std::forward<TArgs>(args)...);
        object.Attach(ptr);
        allocator.Detach();
    }
    return object;
}

template <typename T, typename I, typename ...TArgs>
HRESULT MakeAndInitialize(I** result, TArgs&&... args)
{
    static_assert(__is_base_of(Details::RuntimeClassBase, T), "Make can only instantiate types that derive from RuntimeClass");
    static_assert(__is_base_of(I, T), "The 'T' runtime class doesn't implement 'I' interface");
    *result = nullptr;
    Details::MakeAllocator<T> allocator;
    void* buffer = allocator.Allocate();
    if (buffer == nullptr) { return E_OUTOFMEMORY; }
    auto ptr = new (buffer) T;
    ComPtr<T> object;
    object.Attach(ptr);
    allocator.Detach();
    HRESULT hr = object->RuntimeClassInitialize(std::forward<TArgs>(args)...);
    if (FAILED(hr)) { return hr; }
    if constexpr (std::is_convertible<T*, I*>::value)
    {
        *result = object.Detach();
        return S_OK;
    }
    else
    {
        // Ambiguous base (e.g. IInspectable reachable through several interfaces)
        return object->QueryInterface(__uuidof(I), reinterpret_cast<void**>(result));
    }
}

template <typename T, typename I, typename ...TArgs>
HRESULT MakeAndInitialize(ComPtrRef<ComPtr<I>> ppvObject, TArgs&&... args)
{
    return MakeAndInitialize<T>(ppvObject.ReleaseAndGetAddressOf(), std::forward<TArgs>(args)...);
}

} // namespace Details

using Details::MakeAndInitialize;
using Details::Make;

#pragma endregion // make overloads

namespace Details
{
    inline WeakReferenceImpl* CreateWeakReference(IUnknown* unk)
    {
        return Make<WeakReferenceImpl>(unk).Detach();
    }
}

#pragma region WeakRef
// mingw-w64 <wrl/client.h> does not provide WeakRef/AsWeak, they are defined here.
#ifndef __WRL_WEAKREF_DEFINED__
#define __WRL_WEAKREF_DEFINED__

class WeakRef : public ComPtr<IWeakReference>
{
private:
    void operator->();
protected:
    HRESULT InternalResolve(REFIID riid, IInspectable** inspectable) const noexcept
    {
        *inspectable = nullptr;

        if (ptr_ == nullptr)
        {
            // For compatibility with original release, treat nullptr as successful
            return S_OK;
        }

        return ptr_->Resolve(riid, inspectable);
    }
public:
    Details::ComPtrRef<WeakRef> operator&() noexcept
    {
        return Details::ComPtrRef<WeakRef>(this);
    }

    const Details::ComPtrRef<const WeakRef> operator&() const noexcept
    {
        return Details::ComPtrRef<const WeakRef>(this);
    }

    WeakRef() noexcept : ComPtr(nullptr)
    {
    }

    WeakRef(decltype(nullptr)) noexcept : ComPtr(nullptr)
    {
    }

    WeakRef(IWeakReference* ptr) noexcept : ComPtr(ptr)
    {
    }

    WeakRef(const ComPtr<IWeakReference>& ptr) noexcept : ComPtr(ptr)
    {
    }

    WeakRef(const WeakRef& ptr) noexcept : ComPtr(ptr)
    {
    }

    WeakRef(WeakRef&& ptr) noexcept : ComPtr(static_cast<ComPtr<IWeakReference>&&>(ptr))
    {
    }

    ~WeakRef() noexcept
    {
    }

    // Resolve U interface
    template <typename U>
    HRESULT As(Details::ComPtrRef<ComPtr<U>> ptr) const noexcept
    {
        static_assert(!std::is_same<IWeakReference, U>::value, "IWeakReference cannot resolve IWeakReference object.");
        static_assert(__is_base_of(IInspectable, U), "WeakRef::As() can only be used on types derived from IInspectable");

        return InternalResolve(__uuidof(U), reinterpret_cast<IInspectable**>(ptr.ReleaseAndGetAddressOf()));
    }

    template <typename U>
    HRESULT As(ComPtr<U>* ptr) const noexcept
    {
        static_assert(!std::is_same<IWeakReference, U>::value, "IWeakReference cannot resolve IWeakReference object.");
        static_assert(__is_base_of(IInspectable, U), "WeakRef::As() can only be used on types derived from IInspectable");

        return InternalResolve(__uuidof(U), reinterpret_cast<IInspectable**>(ptr->ReleaseAndGetAddressOf()));
    }

    HRESULT AsIID(REFIID riid, ComPtr<IInspectable>* ptr) const noexcept
    {
        __WRL_ASSERT__(!InlineIsEqualGUID(riid, __uuidof(IWeakReference)));

        return InternalResolve(riid, ptr->ReleaseAndGetAddressOf());
    }

    HRESULT CopyTo(REFIID riid, IInspectable** ptr) const noexcept
    {
        __WRL_ASSERT__(!InlineIsEqualGUID(riid, __uuidof(IWeakReference)));

        return InternalResolve(riid, ptr);
    }

    template <typename U>
    HRESULT CopyTo(U** ptr) const noexcept
    {
        static_assert(__is_base_of(IInspectable, U), "WeakRef::CopyTo() can only be used on types derived from IInspectable");

        return InternalResolve(__uuidof(U), reinterpret_cast<IInspectable**>(ptr));
    }

    HRESULT CopyTo(IWeakReference** ptr) const noexcept
    {
        InternalAddRef();
        *ptr = ptr_;
        return S_OK;
    }

    WeakRef& operator=(const WeakRef& other) noexcept
    {
        ComPtr::operator=(static_cast<const ComPtr&>(other));
        return *this;
    }

    WeakRef& operator=(WeakRef&& other) noexcept
    {
        ComPtr::operator=(static_cast<ComPtr&&>(other));
        return *this;
    }
};

template <typename T>
HRESULT AsWeak(T* p, WeakRef* pWeak) noexcept
{
    static_assert(!std::is_same<IWeakReference, T>::value, "Cannot get IWeakReference object to IWeakReference.");
    ComPtr<IWeakReferenceSource> refSource;

    HRESULT hr = p->QueryInterface(__uuidof(IWeakReferenceSource), reinterpret_cast<void**>(refSource.GetAddressOf()));
    if (FAILED(hr))
    {
        return hr;
    }

    ComPtr<IWeakReference> weakref;
    hr = refSource->GetWeakReference(weakref.GetAddressOf());
    if (FAILED(hr))
    {
        return hr;
    }

    *pWeak = WeakRef(weakref);
    return S_OK;
}

#endif // __WRL_WEAKREF_DEFINED__
#pragma endregion // WeakRef

#pragma region AgileRef
// mingw-w64 <wrl/client.h> does not provide AgileRef/AsAgile, they are defined here.
// Same availability as RoGetAgileReference in mingw-w64 <combaseapi.h> (and the SDK).
#if !defined(__WRL_AGILEREF_DEFINED__) && (NTDDI_VERSION >= NTDDI_WINBLUE) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
#define __WRL_AGILEREF_DEFINED__

class AgileRef : public ComPtr<IAgileReference>
{
protected:
    HRESULT InternalResolve(REFIID riid, void** ptr) const
    {
        *ptr = nullptr;

        if (ptr_ == nullptr)
        {
            // For compatibility with original release, treat nullptr as successful
            return S_OK;
        }

        return ptr_->Resolve(riid, ptr);
    }
public:
    Details::ComPtrRef<AgileRef> operator&() noexcept
    {
        return Details::ComPtrRef<AgileRef>(this);
    }

    const Details::ComPtrRef<const AgileRef> operator&() const noexcept
    {
        return Details::ComPtrRef<const AgileRef>(this);
    }

    AgileRef() noexcept : ComPtr(nullptr)
    {
    }

    AgileRef(decltype(nullptr)) noexcept : ComPtr(nullptr)
    {
    }

    AgileRef(IAgileReference* ptr) noexcept : ComPtr(ptr)
    {
    }

    AgileRef(const ComPtr<IAgileReference>& ptr) noexcept : ComPtr(ptr)
    {
    }

    AgileRef(const AgileRef& ptr) noexcept : ComPtr(ptr)
    {
    }

    AgileRef(AgileRef&& ptr) noexcept : ComPtr(static_cast<ComPtr<IAgileReference>&&>(ptr))
    {
    }

    ~AgileRef() noexcept
    {
    }

    template <typename U>
    HRESULT As(Details::ComPtrRef<ComPtr<U>> ptr) const noexcept
    {
        static_assert(!std::is_same<IAgileReference, U>::value, "IAgileReference cannot resolve IAgileReference object.");
        static_assert(__is_base_of(IUnknown, U), "AgileRef::As() can only be used on types derived from IUnknown");

        return InternalResolve(__uuidof(U), reinterpret_cast<void**>(ptr.ReleaseAndGetAddressOf()));
    }

    template <typename U>
    HRESULT As(ComPtr<U>* ptr) const noexcept
    {
        static_assert(!std::is_same<IAgileReference, U>::value, "IAgileReference cannot resolve IAgileReference object.");
        static_assert(__is_base_of(IUnknown, U), "AgileRef::As() can only be used on types derived from IUnknown");

        return InternalResolve(__uuidof(U), reinterpret_cast<void**>(ptr->ReleaseAndGetAddressOf()));
    }

    HRESULT AsIID(REFIID riid, ComPtr<IUnknown>* ptr) const noexcept
    {
        __WRL_ASSERT__(!InlineIsEqualGUID(riid, __uuidof(IAgileReference)));

        return InternalResolve(riid, reinterpret_cast<void**>(ptr->ReleaseAndGetAddressOf()));
    }

    HRESULT CopyTo(REFIID riid, IUnknown** ptr) const noexcept
    {
        __WRL_ASSERT__(!InlineIsEqualGUID(riid, __uuidof(IAgileReference)));

        return InternalResolve(riid, reinterpret_cast<void**>(ptr));
    }

    template <typename U>
    HRESULT CopyTo(U** ptr) const noexcept
    {
        static_assert(__is_base_of(IUnknown, U), "AgileRef::As() can only be used on types derived from IUnknown");

        return InternalResolve(__uuidof(U), reinterpret_cast<void**>(ptr));
    }

    HRESULT CopyTo(IAgileReference** ptr) const noexcept
    {
        InternalAddRef();
        *ptr = ptr_;
        return S_OK;
    }

    AgileRef& operator=(const AgileRef& other) noexcept
    {
        ComPtr::operator=(static_cast<const ComPtr&>(other));
        return *this;
    }

    AgileRef& operator=(AgileRef&& other) noexcept
    {
        ComPtr::operator=(static_cast<ComPtr&&>(other));
        return *this;
    }

    void operator->() = delete;
};

template <typename T>
HRESULT AsAgile(T* p, AgileRef* pAgile) noexcept
{
    static_assert(!std::is_same<IAgileReference, T>::value, "Cannot get IAgileReference object to IAgileReference.");

    HRESULT hr = S_OK;
    if (p)
    {
        hr = ::RoGetAgileReference(AGILEREFERENCE_DEFAULT, __uuidof(T), p, pAgile->ReleaseAndGetAddressOf());
    }
    else
    {
        *pAgile = nullptr;
    }

    return hr;
}

#endif // __WRL_AGILEREF_DEFINED__
#pragma endregion // AgileRef

#define InspectableClass(runtimeClassName, trustLevel) \
    public: \
        static const wchar_t* STDMETHODCALLTYPE InternalGetRuntimeClassName() noexcept \
        { \
            static_assert((RuntimeClassT::ClassFlags::value & ::Microsoft::WRL::WinRtClassicComMix) == ::Microsoft::WRL::WinRt || \
                (RuntimeClassT::ClassFlags::value & ::Microsoft::WRL::WinRtClassicComMix) == ::Microsoft::WRL::WinRtClassicComMix, \
                    "'InspectableClass' macro must not be used with ClassicCom clasess."); \
            static_assert(__is_base_of(::Microsoft::WRL::Details::RuntimeClassBase, RuntimeClassT), "'InspectableClass' macro can only be used with ::Windows::WRL::RuntimeClass types"); \
            static_assert(!__is_base_of(IActivationFactory, RuntimeClassT), "Incorrect usage of IActivationFactory interface. Make sure that your RuntimeClass doesn't implement IActivationFactory interface use ::Windows::WRL::ActivationFactory instead or 'InspectableClass' macro is not used on ::Windows::WRL::ActivationFactory"); \
            return runtimeClassName; \
        } \
        static ::TrustLevel STDMETHODCALLTYPE InternalGetTrustLevel() noexcept \
        { \
            return trustLevel; \
        } \
        STDMETHOD(GetRuntimeClassName)(HSTRING* runtimeName) override \
        { \
            *runtimeName = nullptr; \
            HRESULT hr = S_OK; \
            auto name = InternalGetRuntimeClassName(); \
            if (name != nullptr) \
            { \
                hr = ::WindowsCreateString(name, static_cast<UINT32>(::wcslen(name)), runtimeName); \
            } \
            return hr; \
        } \
        STDMETHOD(GetTrustLevel)(::TrustLevel* trustLvl) override \
        { \
            *trustLvl = trustLevel; \
            return S_OK; \
        } \
        STDMETHOD(GetIids)(ULONG* iidCount, IID** iids) override \
        { \
            return RuntimeClassT::GetIids(iidCount, iids); \
        } \
        STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject) override \
        { \
            bool handled = false; \
            HRESULT hr = this->CustomQueryInterface(riid, ppvObject, &handled); \
            if (FAILED(hr) || handled) return hr; \
            return RuntimeClassT::QueryInterface(riid, ppvObject); \
        } \
        STDMETHOD_(ULONG, Release)() override \
        { \
            return RuntimeClassT::Release(); \
        } \
        STDMETHOD_(ULONG, AddRef)() override \
        { \
            return RuntimeClassT::AddRef(); \
        } \
    private:

#define MixInHelper() \
    public: \
        STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject) \
        { \
            static_assert((RuntimeClassT::ClassFlags::value & ::Microsoft::WRL::WinRt) == 0, "'MixInClass' macro must not be used with WinRt clasess."); \
            static_assert(__is_base_of(::Microsoft::WRL::Details::RuntimeClassBase, RuntimeClassT), "'MixInHelper' macro can only be used with ::Windows::WRL::RuntimeClass types"); \
            static_assert(!__is_base_of(IClassFactory, RuntimeClassT), "Incorrect usage of IClassFactory interface. Make sure that your RuntimeClass doesn't implement IClassFactory interface use ::Windows::WRL::ClassFactory instead or 'MixInHelper' macro is not used on ::Windows::WRL::ClassFactory"); \
            return RuntimeClassT::QueryInterface(riid, ppvObject); \
        } \
        STDMETHOD_(ULONG, Release)() \
        { \
            return RuntimeClassT::Release(); \
        } \
        STDMETHOD_(ULONG, AddRef)() \
        { \
            return RuntimeClassT::AddRef(); \
        } \
    private:

// Please make sure that those macros are in sync with those ones from 'wrl/module.h'
#ifndef WrlCreatorMapIncludePragmaEx
#define WrlCreatorMapIncludePragmaEx(className, group)  static_assert(false, "It's required to include 'wrl/module.h' to be able to use 'WrlCreatorMapIncludePragmaEx' macro");
#endif

#ifndef WrlCreatorMapIncludePragma
#define WrlCreatorMapIncludePragma(className)  static_assert(false, "It's required to include 'wrl/module.h' to be able to use 'WrlCreatorMapIncludePragma' macro");
#endif

#ifndef ActivatableClassWithFactoryEx
#define ActivatableClassWithFactoryEx(className, factory, groupId)  static_assert(false, "It's required to include 'wrl/module.h' to be able to use 'ActivatableClassWithFactoryEx' macro");
#endif

#ifndef ActivatableClassWithFactory
#define ActivatableClassWithFactory(className, factory)  static_assert(false, "It's required to include 'wrl/module.h' to be able to use 'ActivatableClassWithFactory' macro");
#endif

#ifndef ActivatableClass
#define ActivatableClass(className)  static_assert(false, "It's required to include 'wrl/module.h' to be able to use 'ActivatableClass' macro");
#endif

#ifndef ActivatableStaticOnlyFactoryEx
#define ActivatableStaticOnlyFactoryEx(factory, serverName)  static_assert(false, "It's required to include 'wrl/module.h' to be able to use 'ActivatableStaticOnlyFactoryEx' macro");
#endif

#ifndef ActivatableStaticOnlyFactory
#define ActivatableStaticOnlyFactory(factory)  static_assert(false, "It's required to include 'wrl/module.h' to be able to use 'ActivatableStaticOnlyFactory' macro");
#endif

#ifndef CoCreatableClassWithFactoryEx
#define CoCreatableClassWithFactoryEx(className, factory, groupId)  static_assert(false, "It's required to include 'wrl/module.h' to be able to use 'CoCreatableClassWithFactoryEx' macro");
#endif

#ifndef CoCreatableClassWithFactory
#define CoCreatableClassWithFactory(className, factory)  static_assert(false, "It's required to include 'wrl/module.h' to be able to use 'CoCreatableClassWithFactory' macro");
#endif

#ifndef CoCreatableClass
#define CoCreatableClass(className)  static_assert(false, "It's required to include 'wrl/module.h' to be able to use 'CoCreatableClass' macro");
#endif

#ifndef CoCreatableClassWrlCreatorMapInclude
#define CoCreatableClassWrlCreatorMapInclude(className)  static_assert(false, "It's required to include 'wrl/module.h' to be able to use 'CoCreatableClassWrlCreatorMapInclude' macro");
#endif

#ifndef CoCreatableClassWrlCreatorMapIncludeEx
#define CoCreatableClassWrlCreatorMapIncludeEx(className, groupId)  static_assert(false, "It's required to include 'wrl/module.h' to be able to use 'CoCreatableClassWrlCreatorMapIncludeEx' macro");
#endif

#undef UnknownIncrementReference
#undef UnknownDecrementReference
#undef UnknownBarrierAfterInterlock
#undef UnknownInterlockedCompareExchangePointer
#undef UnknownInterlockedCompareExchangePointerForIncrement
#undef UnknownInterlockedCompareExchangePointerForRelease
#undef UnknownInterlockedCompareExchangeForIncrement
#undef UnknownInterlockedCompareExchangeForRelease

}}    // namespace Microsoft::WRL

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif // _WRL_IMPLEMENTS_H_
