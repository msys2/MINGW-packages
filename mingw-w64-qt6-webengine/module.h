/**
 * wrl/module.h - Microsoft::WRL::Module and class factories for mingw-w64.
 *
 * Port of the Windows SDK <wrl/module.h> to mingw-w64 (clang/gcc, lld or GNU
 * ld). Module<InProc/OutOfProc>, ClassFactory, ActivationFactory and the
 * ActivatableClass / CoCreatableClass registration macros follow the SDK.
 *
 * Differences from the SDK header:
 *  - The creator map lives in ".CRT$XCZ$WRL$*" sections instead of "minATL$*"
 *    (see ModuleBase in <wrl/implements.h>). Entries are always retained by
 *    the linker, even with --gc-sections, so WrlCreatorMapIncludePragma*() and
 *    CoCreatableClassWrlCreatorMapInclude*() expand to nothing.
 *  - A class registered with CoCreatableClass* must have an IID mapping for
 *    __uuidof(className), i.e. __CRT_UUID_DECL(className, ...) instead of
 *    MSVC's class __declspec(uuid(...)).
 *  - Registering the same class (and server name) in two translation units is
 *    a duplicate symbol link error instead of being silently merged.
 *  - Module<>::Create() uses a function local static instead of InitOnce +
 *    StaticStorage.
 *  - The UWP (WINAPI_PARTITION_PC_APP only) Module<OutOfProc> based on
 *    CoreApplication is not provided; the desktop implementation is used.
 */

#ifndef _WRL_MODULE_H_
#define _WRL_MODULE_H_

#include <roapi.h>
#include <activation.h>
#include <winstring.h>
#include <winapifamily.h>

#include <new>
#include <wchar.h>

#include <wrl/internal.h>
#include <wrl/client.h>
#include <wrl/implements.h>
#include <wrl/ftm.h>
#include <wrl/wrappers/corewrappers.h>
#include <wrl/event.h>  // Wrappers::SRWLock, E_ILLEGAL_METHOD_CALL

#ifndef WrlFinal
#define WrlFinal final
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winaccessible-base"
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"
// Module<InProc>::isInitialized and Module<>::Create() mirror the SDK, which
// creates the module during static initialization and destroys it at exit.
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Winaccessible-base"
#pragma GCC diagnostic ignored "-Wsubobject-linkage"
#endif

namespace Microsoft {
namespace WRL {

enum ModuleType
{
    InProc                   = 0x1,  // inproc server,
    OutOfProc                = 0x2,  // outproc server
    DisableCaching           = 0x4,   // disable caching mechanism on Module<>
    InProcDisableCaching     = InProc | DisableCaching,
    OutOfProcDisableCaching  = OutOfProc | DisableCaching
};

enum FactoryCacheFlags
{
    FactoryCacheDefault,
    FactoryCacheEnabled,
    FactoryCacheDisabled
};

namespace Details
{

// Keep information about factory and cookie data
struct FactoryCache
{
    IUnknown* factory;
    union {
        RO_REGISTRATION_COOKIE winrt;
        DWORD com;
    } cookie;
};

// Map contains information how to initialize, register and unregister objects
//
// How to compare activation data depending on classic COM or WinRT factory
// Keeps information about factory cache, server name for interface
struct CreatorMap
{
    // Object id. The constructors allow constant (link time) initialization of
    // either member; the SDK reinterpret_casts the function pointer instead,
    // which is not a constant expression.
    union ActivationId {
        const IID* clsid;
        const wchar_t* (STDMETHODCALLTYPE *getRuntimeName)();

        constexpr ActivationId(const IID* id) noexcept : clsid(id) {}
        constexpr ActivationId(const wchar_t* (STDMETHODCALLTYPE *getName)()) noexcept : getRuntimeName(getName) {}
    };

    // Factory creator function
    HRESULT (STDMETHODCALLTYPE *factoryCreator)(unsigned int*, const CreatorMap*, REFIID, IUnknown**) noexcept;
    // Object id
    ActivationId activationId;
    // Trust level for WinRT otherwise nullptr
    ::TrustLevel (STDMETHODCALLTYPE *getTrustLevel)();
    // Factory cache, group id data members
    FactoryCache* factoryCache;
    const wchar_t* serverName;
};

class FactoryBase
{
};

// Compare server name strings
inline bool IsServerNameEqual(const CreatorMap* entry, const wchar_t* serverName) noexcept
{
    if (serverName == nullptr)
    {
        return true;
    }
    else if (entry->serverName == nullptr)
    {
        return false;
    }

    return ::wcscmp(entry->serverName, serverName) == 0;
}

// Terminate class factories stored in the cache
inline bool TerminateMap(ModuleBase* modulePtr, const wchar_t* serverName, bool forceTerminate) noexcept
{
    auto entry = modulePtr->GetFirstEntryPointer() + 1;
    auto last = modulePtr->GetLastEntryPointer();

    // Walk the linker generated list of pointers to CreatorMap
    // It's necessary to start from COM objects and ends with WinRT
    for (; entry < last; entry++)
    {
        // Linker generated list can have null pointer values
        if (*entry != nullptr && IsServerNameEqual(*entry, serverName))
        {
            // We should not terminate cache if we have objects alive
            if (modulePtr->GetObjectCount() > 0 && !forceTerminate)
            {
                return false;
            }

            if (static_cast<IUnknown* volatile&>((*entry)->factoryCache->factory) == nullptr)
            {
                continue;
            }

            // Make sure that nobody is taking object from cache when we terminate factories
            void* factoryPointer = nullptr;
            { //Open scope for lock
                auto lock = ::Microsoft::WRL::Wrappers::SRWLock::LockExclusive(modulePtr->GetLock());

                // Don't need read memory barrier because lock adds one
                if ((*entry)->factoryCache->factory == nullptr)
                {
                    continue;
                }

                factoryPointer = (*entry)->factoryCache->factory;
                (*entry)->factoryCache->factory = nullptr;
            } // End of lock scope

            __WRL_ASSERT__(factoryPointer != nullptr);
            IUnknown* factory = reinterpret_cast<IUnknown*>(::DecodePointer(factoryPointer));
            factory->Release();
        }
    }

    return modulePtr->GetObjectCount() == 0 ? true : false;
}

// Gets the factory from the cache entry and if not available create one.
inline HRESULT GetCacheEntry(ModuleBase* modulePtr, unsigned int* flags, REFIID riid, const CreatorMap* entry, IUnknown** ppFactory) noexcept
{
    *ppFactory = nullptr;

    IUnknown* factory = nullptr;

    // Check if the object is available in the cache
    if (static_cast<IUnknown* volatile&>(entry->factoryCache->factory) != nullptr)
    { // Read lock scope
        // Make sure that none of factories will be destroyed when WRL gets element from cache
        auto readLock = ::Microsoft::WRL::Wrappers::SRWLock::LockShared(modulePtr->GetLock());

        void* factoryPointer = entry->factoryCache->factory;
        if (factoryPointer != nullptr)
        {
            factory = reinterpret_cast<IUnknown*>(::DecodePointer(factoryPointer));
            __WRL_ASSERT__(factory != nullptr);
            return factory->QueryInterface(riid, reinterpret_cast<void**>(ppFactory));
        }
    } // End of read lock scope

    HRESULT hr = entry->factoryCreator(flags, entry, riid, &factory);
    if (FAILED(hr))
    {
        return hr;
    }

    // If caching enabled
    if ((*flags & DisableCaching) == 0)
    {
        IUnknown* cachedFactory = nullptr;
        { // Write lock scope
            auto writeLock = ::Microsoft::WRL::Wrappers::SRWLock::LockExclusive(modulePtr->GetLock());

            // Don't need read memory barrier because lock adds one
            void* factoryPointer = entry->factoryCache->factory;
            if (factoryPointer == nullptr)
            {
                // Put factory in cache
                entry->factoryCache->factory = reinterpret_cast<IUnknown*>(::EncodePointer(factory));
            }
            else
            {
                // Get factory from the cache if it's already there
                cachedFactory = reinterpret_cast<IUnknown*>(::DecodePointer(factoryPointer));
                cachedFactory->AddRef();
            }
        }  // End of write lock scope

        if (cachedFactory != nullptr)
        {
            // Release factory that was created
            // Requires double release because factoryCreator does two AddRef
            factory->Release();
            factory->Release();
            factory = cachedFactory;
        }
    }

    *ppFactory = factory;
    __WRL_ASSERT__(*ppFactory != nullptr);
    return S_OK;
}

template <unsigned int flags>
inline HRESULT GetClassObject(ModuleBase* modulePtr, const wchar_t* serverName, REFCLSID clsid, REFIID riid, void** ppv) noexcept
{
    *ppv = nullptr;

    auto entry = modulePtr->GetFirstEntryPointer() + 1;
    auto last = modulePtr->GetMidEntryPointer();

    // Walk the linker generated list of pointers to CreatorMap for COM objects
    for (; entry < last; entry++)
    {
        // Linker generated list can have null pointer values
        if (*entry != nullptr && IsServerNameEqual(*entry, serverName))
        {
            if (InlineIsEqualGUID(*(*entry)->activationId.clsid, clsid))
            {
                unsigned int currentFlags = flags;
                if ((flags & DisableCaching) == 0)
                {
                    // Does not require QI
                    return GetCacheEntry(modulePtr, &currentFlags, riid, *entry, reinterpret_cast<IUnknown**>(ppv));
                }
                else
                {
                    return (*entry)->factoryCreator(&currentFlags, *entry, riid, reinterpret_cast<IUnknown**>(ppv));
                }
            }
        }
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}

template <unsigned int flags>
inline HRESULT GetActivationFactory(ModuleBase* modulePtr, const wchar_t* serverName, HSTRING activatibleClassId, IActivationFactory** ppFactory) noexcept
{
    *ppFactory = nullptr;

    BOOL hasEmbedNull;
    if (::WindowsIsStringEmpty(activatibleClassId) ||
        (FAILED(::WindowsStringHasEmbeddedNull(activatibleClassId, &hasEmbedNull)) || hasEmbedNull == TRUE))
    {
        static const WCHAR pszParamName[] = L"activatibleClassId";
        RoOriginateErrorW(E_INVALIDARG, ARRAYSIZE(pszParamName) - 1, pszParamName);
        return E_INVALIDARG;
    }

    const wchar_t* id = ::WindowsGetStringRawBuffer(activatibleClassId, nullptr);

    auto entry = modulePtr->GetMidEntryPointer() + 1;
    auto last = modulePtr->GetLastEntryPointer();

    // Walk the linker generated list of pointers to CreatorMap for WinRT objects
    for (; entry < last; entry++)
    {
        // Linker generated list can have null pointer values
        if (*entry != nullptr && IsServerNameEqual(*entry, serverName))
        {
            const wchar_t* runtimeName = ((*entry)->activationId.getRuntimeName)();
            __WRL_ASSERT__(runtimeName != nullptr);

            if (::wcscmp(id, runtimeName) == 0)
            {
                unsigned int currentFlags = flags;

                if ((flags & DisableCaching) == 0)
                {
                    // Does not require QI
                    return GetCacheEntry(modulePtr, &currentFlags, __uuidof(IActivationFactory), *entry, reinterpret_cast<IUnknown**>(ppFactory));
                }
                else
                {
                    return (*entry)->factoryCreator(&currentFlags, *entry, __uuidof(IActivationFactory), reinterpret_cast<IUnknown**>(ppFactory));
                }
            }
        }
    }

    RoOriginateError(CLASS_E_CLASSNOTAVAILABLE, activatibleClassId);
    return CLASS_E_CLASSNOTAVAILABLE;
}

// Activation callback function that is called when object is requested by WinRT API
template <unsigned int flags>
inline HRESULT STDAPICALLTYPE ActivationFactoryCallback(HSTRING activationId, IActivationFactory** ppFactory) noexcept
{
    auto modulePtr = ::Microsoft::WRL::GetModuleBase();
    __WRL_ASSERT__(modulePtr != nullptr);

    return GetActivationFactory<flags>(modulePtr, nullptr, activationId, ppFactory);
}

template <unsigned int flags>
inline HRESULT RegisterWinRTObject(const wchar_t*, const wchar_t** activatableClassIds, RO_REGISTRATION_COOKIE* cookie, unsigned int count) noexcept
{
    PFNGETACTIVATIONFACTORY* activationFactoryCallbacks = new (std::nothrow) PFNGETACTIVATIONFACTORY[count];
    HSTRING* activatableClassIdsHstring = new (std::nothrow) HSTRING[count];
    HRESULT hr = S_OK;

    if (activationFactoryCallbacks == nullptr || activatableClassIdsHstring == nullptr)
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        unsigned int index = 0;
        for (; index < count && SUCCEEDED(hr); index++)
        {
            activationFactoryCallbacks[index] = &Details::ActivationFactoryCallback<flags>;
            hr = ::WindowsCreateString(activatableClassIds[index], static_cast<UINT32>(::wcslen(activatableClassIds[index])), &activatableClassIdsHstring[index]);
        }

        if (SUCCEEDED(hr))
        {
            hr = ::RoRegisterActivationFactories(activatableClassIdsHstring, activationFactoryCallbacks, count, cookie);
        }

        for (unsigned int i = 0; i < index; i++)
        {
            ::WindowsDeleteString(activatableClassIdsHstring[i]);
        }
    }

    delete [] activationFactoryCallbacks;
    delete [] activatableClassIdsHstring;

    return hr;
}

template <unsigned int comFlags>
inline HRESULT RegisterCOMObject(const wchar_t*, IID* clsids, IClassFactory** factories, DWORD* cookies, unsigned int count) noexcept
{
    HRESULT hr = S_OK;
    unsigned int index = 0;

    for (; index < count && SUCCEEDED(hr); index++)
    {
        hr = ::CoRegisterClassObject(clsids[index], factories[index], CLSCTX_LOCAL_SERVER, comFlags | REGCLS_SUSPENDED, &cookies[index]);
    }

    if (SUCCEEDED(hr))
    {
        //Resume all registered objects
        hr = ::CoResumeClassObjects();
    }

    // Unregister all objects that were already registered
    if (FAILED(hr))
    {
        for (unsigned int i = 0; i < index; i++)
        {
            ::CoRevokeClassObject(cookies[i]);
            cookies[i] = 0;
        }
    }

   return hr;
}

inline unsigned int CountObjectEntries(const CreatorMap** first, const CreatorMap** end, const wchar_t* serverName) noexcept
{
    unsigned int count = 0;

    for (const CreatorMap** entry = first + 1; entry < end; entry++)
    {
        if (*entry != nullptr && IsServerNameEqual(*entry, serverName))
        {
            count++;
        }
    }

    return count;
}

template <unsigned int flags>
inline HRESULT RegisterObjects(ModuleBase* modulePtr, const wchar_t* serverName) noexcept
{
    HRESULT hr = S_OK;

    auto firstEntry = modulePtr->GetFirstEntryPointer();
    auto midEntry = modulePtr->GetMidEntryPointer();

    // Count how many COM objects are in the map
    unsigned int objectCount = CountObjectEntries(firstEntry, midEntry, serverName);

    // COM entries
    if (objectCount > 0)
    {
        // Allocate memory for temporary cookie, factory and clsid's arrays
        DWORD* cookies = new (std::nothrow) DWORD[objectCount];
        IClassFactory** factories = new (std::nothrow) IClassFactory*[objectCount];
        IID* clsids = new (std::nothrow) IID[objectCount];

        if (cookies == nullptr || factories == nullptr || clsids == nullptr)
        {
            hr = E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hr))
        {
            unsigned int index = 0;
            // Instantiate factories and copy clsid to temporary storage
            for (const CreatorMap** entry = firstEntry + 1; entry < midEntry && SUCCEEDED(hr); entry++)
            {
                if (*entry != nullptr && IsServerNameEqual(*entry, serverName))
                {
                    unsigned int currentFlags = flags;
                    IUnknown* factory = nullptr;
                    hr = (*entry)->factoryCreator(&currentFlags, *entry, __uuidof(IClassFactory), &factory);
                    if (SUCCEEDED(hr))
                    {
                        factories[index] = reinterpret_cast<IClassFactory*>(factory);
                        clsids[index] = *(*entry)->activationId.clsid;
                        index++;
                    }
                }
            }

            if (SUCCEEDED(hr))
            {
                // Register COM objects
                hr = modulePtr->RegisterCOMObject(serverName, clsids, factories, cookies, objectCount);
                if (SUCCEEDED(hr))
                {
                    // Store COM cookies in WRL map
                    index = 0;
                    for (const CreatorMap** entry = firstEntry + 1; entry < midEntry; entry++)
                    {
                        if (*entry != nullptr && IsServerNameEqual(*entry, serverName))
                        {
                            (*entry)->factoryCache->cookie.com = cookies[index];
                            index++;
                        }
                    }
                }
            }

            // Release local copy of factories
            for (unsigned int i = 0; i < index; i++)
            {
                factories[i]->Release();
            }
        }

        delete [] cookies;
        delete [] clsids;
        delete [] factories;
    }

    // WinRT entries
    if (SUCCEEDED(hr))
    {
        auto lastEntry = modulePtr->GetLastEntryPointer();

        // Count how many WinRT objects are in the map
        objectCount = CountObjectEntries(midEntry, lastEntry, serverName);

        if (objectCount > 0)
        {
            // Create local storage for activatable class ids
            const wchar_t** activatableClassIds = new (std::nothrow) const wchar_t*[objectCount];

            if (activatableClassIds == nullptr)
            {
                hr = E_OUTOFMEMORY;
            }

            if (SUCCEEDED(hr))
            {
                RO_REGISTRATION_COOKIE cookie = nullptr;
                // Copy activatable class ids from WRL creator map
                unsigned int classCount = 0;
                for (const CreatorMap** entry = midEntry + 1; entry < lastEntry; entry++)
                {
                    if (*entry != nullptr && IsServerNameEqual(*entry, serverName))
                    {
                        const wchar_t* id = ((*entry)->activationId.getRuntimeName)();
                        __WRL_ASSERT__(id != nullptr);
                        activatableClassIds[classCount] = id;
                        classCount++;
                    }
                }

                hr = modulePtr->RegisterWinRTObject(serverName, activatableClassIds, &cookie, classCount);
                if (SUCCEEDED(hr))
                {
                    // Copy cookie to the map
                    for (const CreatorMap** entry = midEntry + 1; entry < lastEntry; entry++)
                    {
                        if (*entry != nullptr && IsServerNameEqual(*entry, serverName))
                        {
                             (*entry)->factoryCache->cookie.winrt = cookie;
                        }
                    }
                }
            }

            delete [] activatableClassIds;
        }
    }

    return hr;
}

inline HRESULT UnregisterObjects(ModuleBase* modulePtr, const wchar_t* serverName) noexcept
{
    HRESULT hr = S_OK;

    auto firstEntry = modulePtr->GetFirstEntryPointer();
    auto midEntry = modulePtr->GetMidEntryPointer();

    // Count how many COM objects are in the map
    unsigned int objectCount = CountObjectEntries(firstEntry, midEntry, serverName);

    //COM entries
    if (objectCount > 0)
    {
        // Allocate temporary array for COM cookies
        DWORD* cookies = new (std::nothrow) DWORD[objectCount];

        if (cookies == nullptr)
        {
            hr = E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hr))
        {
            // Copy all COM cookies to temporary array
            unsigned int index = 0;
            for (const CreatorMap** entry = firstEntry + 1; entry < midEntry; entry++)
            {
                if (*entry != nullptr && IsServerNameEqual(*entry, serverName))
                {
                    cookies[index] = (*entry)->factoryCache->cookie.com;
                    index++;
                }
            }

            // Unregister COM objects
            hr = modulePtr->UnregisterCOMObject(serverName, cookies, objectCount);

            // Copy all cookies back to WRL entry map
            index = 0;
            for (const CreatorMap** entry = firstEntry + 1; entry < midEntry; entry++)
            {
                if (*entry != nullptr && IsServerNameEqual(*entry, serverName))
                {
                    (*entry)->factoryCache->cookie.com = cookies[index];
                    index++;
                }
            }
        }

        delete [] cookies;
    }

    // WinRT entries
    if (SUCCEEDED(hr))
    {
        RO_REGISTRATION_COOKIE cookie = nullptr;
        bool foundCookie = false;
        auto lastEntry = modulePtr->GetLastEntryPointer();

        // Get the cookie for the server, all cookies are the same for specific server thus it's enough to find first entry and abort
        for (const CreatorMap** entry = midEntry + 1; entry < lastEntry; entry++)
        {
            if (*entry != nullptr && IsServerNameEqual(*entry, serverName))
            {
                 cookie = (*entry)->factoryCache->cookie.winrt;
                 foundCookie = true;
                 break;
            }
        }

        // If valid cookie for the server found WRL can unregister WinRt objects
        if (foundCookie)
        {
            // Unregister WinRt objects
            hr = modulePtr->UnregisterWinRTObject(serverName, cookie);

            if (SUCCEEDED(hr))
            {
                // Reset cookies if unregister succeeded
                for (const CreatorMap** entry = midEntry + 1; entry < lastEntry; entry++)
                {
                    if (*entry != nullptr && IsServerNameEqual(*entry, serverName))
                    {
                         (*entry)->factoryCache->cookie.winrt = nullptr;
                    }
                }
            }
        }
    }

    // Release all factories
    TerminateMap(modulePtr, serverName, false);
    return hr;
}

// Class factory creator function is templated on factory type instantiate the factory according to flags settings
// that changes caching/ref counting behavior. There are following scenarios available:
// 1) User is creating factory with Make<ClassFactory> than:
//       - no object counting on Module<> is enabled if user want to lock server it's required to call LockServer method on factory
//       - caching disabled
// 2) OutOfProc server
//       - no object counting on Module<> is enabled
//       - caching disabled
//       - DisableCaching flag doesn't change behavior
// 3) InProc server
//       - caching enabled
//       - object counting on Module<> is enabled
//       - when ref count in factory AddRef reaches 2 WRL increments object count
//       - when ref count in factory Release reaches 1 WRL decrements object count
// 4) InProc | DisabledCaching
//       - caching disabled
//       - object counting on Module<> is enabled
//       - object count is incremented when CreateClassFactory function create factory
//       - object count is decremented when ref count on factory Release reaches 0
//
// When caching is enabled CreateClassFactory function always return ppFactory with ref count 2 otherwise 1
template <typename Factory>
inline HRESULT STDMETHODCALLTYPE CreateClassFactory(unsigned int* flags, const CreatorMap*, REFIID riid, IUnknown** ppFactory) noexcept
{
    static_assert(__is_base_of(IClassFactory, Factory), "'Factory' must inherit from 'IClassFactory'");
    static_assert(__is_base_of(FactoryBase, Factory), "'Factory' must inherit from '::Microsoft::WRL::ClassFactory'");

    // Set factory flags that will enable/disable caching behavior depending on flags specified on factories
    switch (Factory::cacheFlag)
    {
        case FactoryCacheFlags::FactoryCacheEnabled:
            if ((*flags & DisableCaching) != 0)
            {
                __WRL_ASSERT__(false && "Mismatched Module<> and 'Factory' configuration. 'Factory' is cacheable and Module<> doesn't support caching");
                *flags |= DisableCaching;
            }
            else
            {
                *flags &= ~DisableCaching;
            }
            break;
        case FactoryCacheFlags::FactoryCacheDisabled:
            *flags |= DisableCaching;
            break;
        default:
            break;
    }

    ComPtr<Factory> classFactory;
    HRESULT hr = MakeAndInitialize<Factory>(classFactory.GetAddressOf());
    if (FAILED(hr))
    {
        return hr;
    }

    classFactory.Get()->flags_ = *flags;

    hr = classFactory.CopyTo(riid, reinterpret_cast<void**>(ppFactory));
    if ((*flags & InProc) != 0)
    {
        if (SUCCEEDED(hr))
        {
            if ((*flags & DisableCaching) != 0)
            {
                auto modulePtr = ::Microsoft::WRL::GetModuleBase();
                __WRL_ASSERT__(modulePtr != nullptr);
                // Need increment object count in case the object is not cached
                modulePtr->IncrementObjectCount();
            }
            else
            {
                // Make sure that we will not call release on factory that will cause
                // decrement object and call to release callback in out of proc server
                classFactory.Detach();
            }
        }
        else
        {
            // In case QI fail make sure that ClassFactory will not decrement object count
            classFactory.Get()->flags_ &= ~(InProc | DisableCaching);
        }
    }

    return hr;
}

// Activation factory creator function is templated on factory type. Instantiate the factory according to flags settings
// that changes caching/ref counting behavior. There are following scenarios available:
// 1) User is creating factory with Make<ActivationFactory> than:
//       - object counting on Module<> is enabled if Module<> was instantiated
//       - object count is incremented in constructor of ActivationFactory if Module<> was instantiated
//       - object count is decremented in Release when ref count on factory is equal 0 if Module<> was instantiated
//       - no caching
// 2) OutOfProc server
//       - object counting on Module<> is enabled
//       - caching enabled
//       - when ref count in factory AddRef reaches 2 WRL increments object count
//       - when ref count in factory Release reaches 1 WRL decrements object count
// 3) OutOfProc | DisabledCaching server
//       - object counting on Module<> is enabled
//       - caching disabled
//       - object count is incremented in constructor of ActivationFactory
//       - object count is decremented in Release when ref count on factory is equal 0
// 4) InProc server
//       - caching enabled
//       - object counting on Module<> is enabled
//       - when ref count in factory AddRef reaches 2 WRL increments object count
//       - when ref count in factory Release reaches 1 WRL decrements object count
// 5) InProc | DisabledCaching
//       - caching disabled
//       - object counting on Module<> is enabled
//       - object count is incremented in constructor of ActivationFactory
//       - object count is decremented when ref count on factory Release reaches 0
//
// When caching is enabled CreateActivationFactory function always return ppFactory with ref count 2 otherwise 1
template <typename Factory>
inline HRESULT STDMETHODCALLTYPE CreateActivationFactory(unsigned int* flags, const CreatorMap* entry, REFIID riid, IUnknown** ppFactory) noexcept
{
    static_assert(__is_base_of(IActivationFactory, Factory), "'Factory' must inherit from 'IActivationFactory'");
    static_assert(__is_base_of(FactoryBase, Factory), "'Factory' must inherit from '::Microsoft::WRL::IActivationFactory'");

    // Set factory flags that will enable/disable caching behavior depending on flags specified on factories
    switch (Factory::cacheFlag)
    {
        case FactoryCacheFlags::FactoryCacheEnabled:
            if ((*flags & DisableCaching) != 0)
            {
                __WRL_ASSERT__(false && "Mismatched Module<> and 'Factory' configuration. 'Factory' is cacheable and Module<> doesn't support caching");
                *flags |= DisableCaching;
            }
            else
            {
                *flags &= ~DisableCaching;
            }
            break;
        case FactoryCacheFlags::FactoryCacheDisabled:
            *flags |= DisableCaching;
            break;
        default:
            break;
    }

    ComPtr<Factory> activationFactory;
    HRESULT hr = MakeAndInitialize<Factory>(activationFactory.GetAddressOf());
    if (FAILED(hr))
    {
        return hr;
    }

    // QI without calling AddRef because we incremented object count in constructor of Factory
    // The factory always implements IActivationFactory thus this should succeed always because riid is __uuidof(IActivationFactory)
    hr = activationFactory->CanCastTo(riid, reinterpret_cast<void**>(ppFactory));
    if (FAILED(hr))
    {
        return hr;
    }

    // When cache is enabled it's required to increment ref count to 2 for ActivationFactory
    // because CanCastTo didn't do it
    if ((*flags & DisableCaching) == 0)
    {
        activationFactory->InternalAddRef();
    }

    // Set factory flags that will enable/disable caching behavior
    activationFactory.Get()->flags_ = *flags;
    // Make sure that entry is nulled
    __WRL_ASSERT__(activationFactory.Get()->entry_ == nullptr);
    // Assign entry information to get corresponding runtime class name and trust level
    activationFactory.Get()->entry_ = const_cast<CreatorMap*>(entry);
    // Detach factory if CanCastTo succeeded
    activationFactory.Detach();

    return S_OK;
}

#ifdef _DEBUG
template <typename T>
inline void CheckForDuplicateEntries(const CreatorMap** firstEntry, const CreatorMap** lastEntry, T validateEntry) noexcept
{
    __WRL_ASSERT__(firstEntry <= lastEntry);
    if (firstEntry == lastEntry)
    {
        return;
    }

    for (const CreatorMap** entry = firstEntry; entry < (lastEntry - 1); entry++)
    {
        if (*entry == nullptr)
        {
            continue;
        }
        // Walk the linker generated list of pointers to CreatorMap
        for (const CreatorMap** entry2 = (entry + 1); entry2 < lastEntry; entry2++)
        {
            if (*entry2 != nullptr)
            {
                (validateEntry)(*entry, *entry2);
            }
        }
    }
}
#endif // _DEBUG

} // namespace Details

// ClassFactory implementation provides registration methods and basic functionality for IClassFactory interface
// It enables developer to provide custom factory implementation.
// Example:
// struct MyClassFactory : public ClassFactory<IMyAddtionalInterfaceOnFactory>
// {
//        STDMETHOD(CreateInstance)(IUnknown* pUnkOuter, REFIID riid, void** ppvObject)
//        {
//            my custom implementation
//        }
// };
// CoCreatableClassWithFactory(MyClass, MyClassFactory)
//
// When more then 3 interfaces are required to be implemented on factory than:
// struct MyFactory : ClassFactory<Implements<I1, I2, I3>, I4, I5>
template <typename I0 = Details::Nil, typename I1 = Details::Nil, typename I2 = Details::Nil, FactoryCacheFlags cacheFlagValue = FactoryCacheDefault>
class ClassFactory :
    public Details::RuntimeClass<typename Details::InterfaceListHelper<IClassFactory, I0, I1, I2, Details::Nil>::TypeT, RuntimeClassFlags<ClassicCom | InhibitWeakReference>, false>,
    private Details::FactoryBase
{
private:
    static const unsigned int cacheFlag = cacheFlagValue;
public:
    ClassFactory() noexcept : flags_(DisableCaching)
    {
    }

    // IUnknown methods
    STDMETHOD_(ULONG, AddRef)()
    {
        auto refcount = Super::InternalAddRef();

        // Increment object count only when InProc and caching enabled
        if ((flags_ & (OutOfProc | DisableCaching)) == 0 && refcount == 2)
        {
            auto modulePtr = ::Microsoft::WRL::GetModuleBase();
            __WRL_ASSERT__(modulePtr != nullptr);

            if (modulePtr != nullptr)
            {
                modulePtr->IncrementObjectCount();
            }
        }

        return refcount;
    }

    STDMETHOD_(ULONG, Release)()
    {
        auto refcount = Super::InternalRelease();

        if (refcount == 0)
        {
            bool isInProcWithoutCaching = (flags_ & (InProc | DisableCaching)) == (InProc | DisableCaching);
            delete this;

            // Decrement object count only when InProc without caching
            if (isInProcWithoutCaching)
            {
                auto modulePtr = ::Microsoft::WRL::GetModuleBase();
                __WRL_ASSERT__(modulePtr != nullptr);

                if (modulePtr != nullptr)
                {
                    modulePtr->DecrementObjectCount();
                }
            }
        }
        // Decrement object count when InProc and caching enabled
        else if ((flags_ & (OutOfProc | DisableCaching)) == 0 && refcount == 1)
        {
            auto modulePtr = ::Microsoft::WRL::GetModuleBase();
            __WRL_ASSERT__(modulePtr != nullptr);

            if (modulePtr != nullptr)
            {
                modulePtr->DecrementObjectCount();
            }
        }

        return static_cast<ULONG>(refcount);
    }

    STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject)
    {
        return Super::AsIID(this, riid, ppvObject);
    }

    // IClassFactory method
    STDMETHOD(LockServer)(BOOL fLock)
    {
        auto modulePtr = ::Microsoft::WRL::GetModuleBase();
        if (modulePtr == nullptr)
        {
            Details::RoOriginateError(E_FAIL, nullptr);
            return E_FAIL;
        }

        if (fLock)
        {
            modulePtr->IncrementObjectCount();
        }
        else
        {
            modulePtr->DecrementObjectCount();
        }

        return S_OK;
    }

    // Factory creation mechanism is internal for WRL thus declared outside factory as friend
    template <typename Factory> friend HRESULT STDMETHODCALLTYPE Details::CreateClassFactory(unsigned int*, const Details::CreatorMap*, REFIID, IUnknown**) noexcept;

protected:
    using Super = Details::RuntimeClass<typename Details::InterfaceListHelper<IClassFactory, I0, I1, I2, Details::Nil>::TypeT, RuntimeClassFlags<ClassicCom | InhibitWeakReference>, false>;

private:
    unsigned int flags_;
};

// SimpleClassFactory provides basic creation mechanism for base class
// Base class must provide default constructor
// Example:
// CoCreatableClassWithFactoryEx(MyClass, SimpleClassFactory<MyClass>, 1)
template <typename Base, FactoryCacheFlags cacheFlagValue = FactoryCacheDefault>
class SimpleClassFactory : public ClassFactory<Details::Nil, Details::Nil, Details::Nil, cacheFlagValue>
{
public:
    // IClassFactory method
    STDMETHOD(CreateInstance)(IUnknown* pUnkOuter, REFIID riid, void** ppvObject)
    {
#ifdef __WRL_STRICT__
        static_assert(__is_base_of(Details::RuntimeClassBase, Base), "SimpleClassFactory can only instantiate 'Base' that derive from RuntimeClass");
        static_assert((Base::ClassFlags::value & ::Microsoft::WRL::ClassicCom) == ::Microsoft::WRL::ClassicCom,
            "SimpleClassFactory can only instantiate 'Base' that is configured with ClassicCom or WinRtClassicComMix flags");
#endif

        *ppvObject = nullptr;

        if (pUnkOuter != nullptr)
        {
            Details::RoOriginateError(CLASS_E_NOAGGREGATION, nullptr);
            return CLASS_E_NOAGGREGATION;
        }

        ComPtr<IUnknown> unk;
        HRESULT hr = MakeAndInitialize<Base>(unk.GetAddressOf());
        if (FAILED(hr))
        {
            return hr;
        }

        return unk.CopyTo(riid, ppvObject);
    }
};

// Deprecated, use AgileActivationFactory instead.
template <typename I0 = Details::Nil, typename I1 = Details::Nil, typename I2 = Details::Nil, FactoryCacheFlags cacheFlagValue = FactoryCacheDefault>
class ActivationFactory :
    public Details::RuntimeClass<typename Details::InterfaceListHelper<IActivationFactory, I0, I1, I2, Details::Nil>::TypeT, RuntimeClassFlags<WinRt | InhibitWeakReference | InhibitFtmBase>, false>,
    private Details::FactoryBase
{
private:
    static const unsigned int cacheFlag = cacheFlagValue;
public:
    typedef ActivationFactory ActivationFactoryT;
    typedef I0 FirstInterface;

    ActivationFactory() noexcept : entry_(nullptr), flags_(DisableCaching)
    {
        auto modulePtr = ::Microsoft::WRL::GetModuleBase();
        if (modulePtr != nullptr)
        {
            modulePtr->IncrementObjectCount();
        }
    }

    // IUnknown methods
    STDMETHOD_(ULONG, AddRef)()
    {
        auto refcount = Super::InternalAddRef();

        // When caching enabled we increment object count on factory when refcount reaches 2
        if ((flags_ & DisableCaching) == 0 && refcount == 2)
        {
            auto modulePtr = ::Microsoft::WRL::GetModuleBase();
            __WRL_ASSERT__(modulePtr != nullptr);

            if (modulePtr != nullptr)
            {
                modulePtr->IncrementObjectCount();
            }
        }

        return refcount;
    }

    STDMETHOD_(ULONG, Release)()
    {
        auto refcount = Super::InternalRelease();

        if (refcount == 0)
        {
            bool isCacheDisabled = (flags_ & DisableCaching) != 0;
            delete this;

            auto modulePtr = ::Microsoft::WRL::GetModuleBase();
            if (isCacheDisabled && modulePtr != nullptr)
            {
                modulePtr->DecrementObjectCount();
            }
        }
        // When caching enabled WRL decrement object count on factory when it reaches 1
        else if ((flags_ & DisableCaching) == 0 && refcount == 1)
        {
            auto modulePtr = ::Microsoft::WRL::GetModuleBase();
            __WRL_ASSERT__(modulePtr != nullptr);
            if (modulePtr != nullptr)
            {
                modulePtr->DecrementObjectCount();
            }
        }

        return static_cast<ULONG>(refcount);
    }

    STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject)
    {
        return Super::AsIID(this, riid, ppvObject);
    }
    // IInspectable methods
    STDMETHOD(GetIids)(ULONG* iidCount, IID** iids)
    {
        return Super::GetImplementedIIDS(this, iidCount, iids);
    }
    // Factory runtime class name is the same as RuntimeClass that it is exposing
    STDMETHOD(GetRuntimeClassName)(HSTRING* runtimeName)
    {
        *runtimeName = nullptr;
        Details::RoOriginateError(E_ILLEGAL_METHOD_CALL, nullptr);
        return E_ILLEGAL_METHOD_CALL;
    }
    // Factory trust level is the same as RuntimeClass that it is exposing
    STDMETHOD(GetTrustLevel)(::TrustLevel* trustLvl)
    {
        if (entry_ != nullptr)
        {
            *trustLvl = (entry_->getTrustLevel)();
        }
        else
        {
            __WRL_ASSERT__(false && "Use 'InspectableClassStatic' on static ONLY factories or override 'GetTrustLevel' method to set trust level.");
            *trustLvl = ::TrustLevel::FullTrust;
        }

        return S_OK;
    }
    // IActivationFactory method
    STDMETHOD(ActivateInstance)(IInspectable** ppvObject)
    {
        *ppvObject = nullptr;
        Details::RoOriginateError(E_NOTIMPL, nullptr);
        return E_NOTIMPL;
    }
    // Factory creation mechanism is internal for WRL thus declared outside factory as friend
    template <typename Factory> friend HRESULT STDMETHODCALLTYPE Details::CreateActivationFactory(unsigned int*, const Details::CreatorMap*, REFIID, IUnknown**) noexcept;

protected:
    using Super = Details::RuntimeClass<typename Details::InterfaceListHelper<IActivationFactory, I0, I1, I2, Details::Nil>::TypeT, RuntimeClassFlags<WinRt | InhibitWeakReference | InhibitFtmBase>, false>;

private:
    Details::CreatorMap* entry_;
    unsigned int flags_;
};

// AgileActivationFactory implementation provides registration methods and basic functionality for IActivationFactory interface
// It enables developer to provide custom factory implementation.
// Example:
// struct MyClassFactory : public AgileActivationFactory<IMyAddtionalInterfaceOnFactory>
// {
//     IFACEMETHODIMP ActivateInstance(IInspectable** result) override
//     {
//         // custom implementation
//     }
// };
// ActivatableClassWithFactory(MyClass, MyClassFactory)
// or if default factory is used
// ActivatableClassWithFactory(MyClass, SimpleActivationFactory<MyClass>)
//
// When more than 3 interfaces are required to be implemented on factory then:
// struct MyFactory : AgileActivationFactory<Implements<I1, I2, I3>, I4, I5>
template <typename I0 = Details::Nil, typename I1 = Details::Nil, typename I2 = Details::Nil, FactoryCacheFlags cacheFlagValue = FactoryCacheDefault>
class AgileActivationFactory : public ActivationFactory<Implements<FtmBase, I0>, I1, I2, cacheFlagValue>
{
};

// SimpleActivationFactory provides basic creation mechanism for base class
// Base class must provide default constructor
// Example:
// ActivatableClassWithFactoryEx(MyClass, SimpleActivationFactory<MyClass>, 1)
template <typename Base, FactoryCacheFlags cacheFlagValue = FactoryCacheDefault>
class SimpleActivationFactory : public ActivationFactory<Details::Nil, Details::Nil, Details::Nil, cacheFlagValue>
{
public:
    // IActivationFactory method
    STDMETHOD(ActivateInstance)(IInspectable** ppvObject)
    {
#ifdef __WRL_STRICT__
        static_assert(__is_base_of(Details::RuntimeClassBase, Base), "SimpleActivationFactory can only instantiate 'Base' that derive from RuntimeClass");
        static_assert((Base::ClassFlags::value & ::Microsoft::WRL::WinRt) == ::Microsoft::WRL::WinRt,
            "SimpleActivationFactory can only instantiate 'Base' that is configured with WinRt or WinRtClassicComMix flags");
#endif

        return MakeAndInitialize<Base>(ppvObject);
    }
};

template <typename Base, FactoryCacheFlags cacheFlagValue = FactoryCacheDefault>
class SimpleSealedActivationFactory WrlFinal : public SimpleActivationFactory<Base, cacheFlagValue>
{
};

// Agile alternative to SimpleActivationFactory
template <typename Base, FactoryCacheFlags cacheFlagValue = FactoryCacheDefault>
class SimpleAgileActivationFactory : public AgileActivationFactory<Details::Nil, Details::Nil, Details::Nil, cacheFlagValue>
{
public:
    STDMETHOD(ActivateInstance)(IInspectable** ppvObject)
    {
#ifdef __WRL_STRICT__
        static_assert(__is_base_of(Details::RuntimeClassBase, Base), "SimpleAgileActivationFactory can only instantiate 'Base' that derive from RuntimeClass");
        static_assert((Base::ClassFlags::value & ::Microsoft::WRL::WinRt) == ::Microsoft::WRL::WinRt,
            "SimpleAgileActivationFactory can only instantiate 'Base' that is configured with WinRt or WinRtClassicComMix flags");
#endif

        return MakeAndInitialize<Base>(ppvObject);
    }
};

// Agile alternative to SimpleSealedActivationFactory
template <typename Base, FactoryCacheFlags cacheFlagValue = FactoryCacheDefault>
class SimpleSealedAgileActivationFactory WrlFinal : public SimpleAgileActivationFactory<Base, cacheFlagValue>
{
};

// It's required to #undef following macros because they are always defined in 'wrl/implements.h'
// for better error recognition when somebody forgets to include 'wrl/module.h'
// Please make sure that they are in sync with the version from 'wrl/implements.h'
#undef ActivatableClassWithFactoryEx
#undef ActivatableClassWithFactory
#undef ActivatableClass
#undef ActivatableStaticOnlyFactoryEx
#undef ActivatableStaticOnlyFactory
#undef WrlCreatorMapIncludePragma
#undef WrlCreatorMapIncludePragmaEx

#undef CoCreatableClassWithFactoryEx
#undef CoCreatableClassWithFactory
#undef CoCreatableClass
#undef CoCreatableClassWrlCreatorMapInclude
#undef CoCreatableClassWrlCreatorMapIncludeEx

// The SDK forces a reference to every creator map entry with /include: so
// that /OPT:REF does not drop them. The mingw-w64 creator map entries are
// never discarded by lld or GNU ld (see ModuleBase in <wrl/implements.h>), so
// these are no-ops kept for source compatibility.
#define WrlCreatorMapIncludePragma(className)
#define WrlCreatorMapIncludePragmaEx(className, serverName)

// COM specific
#define CoCreatableClassWrlCreatorMapInclude(className) WrlCreatorMapIncludePragma(className##_COM)
#define CoCreatableClassWrlCreatorMapIncludeEx(className, serverName) WrlCreatorMapIncludePragmaEx(className##_COM, serverName)

// 'group' is "__f" for COM and "__r" for WinRT entries. Each entry gets its own
// section ("<group>$<name>") so that entries never fold into each other.
#define InternalWrlCreateCreatorMapEx(className, serverName, runtimeClassName, trustLevel, creatorFunction, group) \
    __declspec(selectany) ::Microsoft::WRL::Details::FactoryCache __objectFactory__##className##_##serverName = { nullptr, { nullptr } }; \
    __declspec(selectany) extern const ::Microsoft::WRL::Details::CreatorMap __object_##className##_##serverName = { \
        creatorFunction, \
        { runtimeClassName }, \
        trustLevel, \
        &__objectFactory__##className##_##serverName,\
        L## #serverName}; \
    extern "C" __WRL_CREATOR_MAP_SECTION__(group "$" #className "_" #serverName) const ::Microsoft::WRL::Details::CreatorMap* const __minATLObjMap_##className##_##serverName = &__object_##className##_##serverName; \
    WrlCreatorMapIncludePragmaEx(className, serverName)

#define InternalWrlCreateCreatorMap(className, runtimeClassName, trustLevel, creatorFunction, group) \
    __declspec(selectany) ::Microsoft::WRL::Details::FactoryCache __objectFactory__##className = { nullptr, { nullptr } }; \
    __declspec(selectany) extern const ::Microsoft::WRL::Details::CreatorMap __object_##className = { \
        creatorFunction, \
        { runtimeClassName }, \
        trustLevel, \
        &__objectFactory__##className,\
        nullptr}; \
    extern "C" __WRL_CREATOR_MAP_SECTION__(group "$" #className) const ::Microsoft::WRL::Details::CreatorMap* const __minATLObjMap_##className = &__object_##className; \
    WrlCreatorMapIncludePragma(className)

// Server name used on ActivatableClassWithFactoryEx or CoCreatableClassWithFactoryEx is used to filter objects registered on the module
// during registration/unregistration or acquiring objects
// The ActivatableClass, ActivatableClassWithFactory, CoCreatableClass and CoCreatableClassWithFactory use serverName id 0.

// Activation macros specific for WinRT
#ifndef __WRL_CLASSIC_COM_STRICT__
#define ActivatableClassWithFactoryEx(className, factory, serverName) \
    InternalWrlCreateCreatorMapEx(className, serverName, &className::InternalGetRuntimeClassName, &className::InternalGetTrustLevel, ::Microsoft::WRL::Details::CreateActivationFactory<factory>, "__r")

#define ActivatableClassWithFactory(className, factory) \
    InternalWrlCreateCreatorMap(className, &className::InternalGetRuntimeClassName, &className::InternalGetTrustLevel, ::Microsoft::WRL::Details::CreateActivationFactory<factory>, "__r")

#define ActivatableClass(className) \
    ActivatableClassWithFactory(className, ::Microsoft::WRL::SimpleSealedActivationFactory<className>)

#define AgileActivatableClass(className) \
    ActivatableClassWithFactory(className, ::Microsoft::WRL::SimpleSealedAgileActivationFactory<className>)

#define ActivatableStaticOnlyFactoryEx(factory, serverName) \
    InternalWrlCreateCreatorMapEx(factory, serverName, &factory::InternalGetRuntimeClassNameStatic, &factory::InternalGetTrustLevelStatic, ::Microsoft::WRL::Details::CreateActivationFactory<factory>, "__r")

#define ActivatableStaticOnlyFactory(factory) \
    InternalWrlCreateCreatorMap(factory, &factory::InternalGetRuntimeClassNameStatic, &factory::InternalGetTrustLevelStatic, ::Microsoft::WRL::Details::CreateActivationFactory<factory>, "__r")

#define InspectableClassStatic(runtimeClassName, trustLevel) \
    public: \
        static const wchar_t* STDMETHODCALLTYPE InternalGetRuntimeClassNameStatic() noexcept \
        { \
            static_assert(__is_base_of(IActivationFactory, ActivationFactoryT) && __is_base_of(::Microsoft::WRL::Details::FactoryBase, ActivationFactoryT), "'InspectableClassStatic' macro can only be used with ::Windows::WRL::ActivationFactory types"); \
            static_assert(!__is_base_of(ActivationFactoryT::FirstInterface, ::Microsoft::WRL::Details::Nil), "ActivationFactory with 'InspectableClassStatic' macro requires to specify custom interfaces"); \
            return runtimeClassName; \
        } \
        static ::TrustLevel STDMETHODCALLTYPE InternalGetTrustLevelStatic() noexcept \
        { \
            return trustLevel; \
        } \
        STDMETHOD(GetRuntimeClassName)(HSTRING* runtimeName) \
        { \
            *runtimeName = nullptr; \
            return E_ILLEGAL_METHOD_CALL; \
        } \
        STDMETHOD(GetTrustLevel)(::TrustLevel* trustLvl) \
        { \
            *trustLvl = trustLevel; \
            return S_OK; \
        } \
        STDMETHOD(GetIids)(ULONG* iidCount, IID** iids) \
        { \
            return ActivationFactoryT::GetIids(iidCount, iids); \
        } \
        STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject) \
        { \
            return ActivationFactoryT::QueryInterface(riid, ppvObject); \
        } \
        STDMETHOD_(ULONG, Release)() \
        { \
            return ActivationFactoryT::Release(); \
        } \
        STDMETHOD_(ULONG, AddRef)() \
        { \
            return ActivationFactoryT::AddRef(); \
        } \
    private:

#else
// When there is classic com only defined those macros should never be used
#define ActivatableClassWithFactoryEx(className, factory, serverName) \
    static_assert(false, "Activation of COM components. Please make sure that that you either undefine __WRL_WINRT_STRICT__ or remove ActivatableClassWithFactoryEx macro");
#define ActivatableClassWithFactory(className, factory) \
    static_assert(false, "Activation of COM components. Please make sure that that you either undefine __WRL_WINRT_STRICT__ or remove ActivatableClassWithFactory macro");
#define ActivatableClass(className) \
    static_assert(false, "Activation of COM components. Please make sure that that you either undefine __WRL_WINRT_STRICT__ or remove ActivatableClass macro");
#define ActivatableStaticOnlyFactoryEx(factory, serverName) \
    static_assert(false, "Activation of COM components. Please make sure that that you either undefine __WRL_WINRT_STRICT__ or remove  macro");
#define ActivatableStaticOnlyFactory(factory) \
    static_assert(false, "Activation of COM components. Please make sure that that you either undefine __WRL_WINRT_STRICT__ or remove ActivatableStaticOnlyFactory macro");
#endif

// Activation macros specific for COM
// mingw-w64: className needs an IID mapping, e.g. __CRT_UUID_DECL(className, ...)
#ifndef __WRL_WINRT_STRICT__
#define CoCreatableClassWithFactoryEx(className, factory, serverName) \
    InternalWrlCreateCreatorMapEx(className##_COM, serverName, &__uuidof(className), nullptr, ::Microsoft::WRL::Details::CreateClassFactory<factory>, "__f")

#define CoCreatableClassWithFactory(className, factory) \
    InternalWrlCreateCreatorMap(className##_COM, &__uuidof(className), nullptr, ::Microsoft::WRL::Details::CreateClassFactory<factory>, "__f")

#define CoCreatableClass(className) \
    CoCreatableClassWithFactory(className, ::Microsoft::WRL::SimpleClassFactory<className>)

#else
// When there is WINRT strict only defined those macros should never be used
#define CoCreatableClassWithFactoryEx(className, factory, serverName) \
    static_assert(false, "Activation of COM components. Please make sure that that you either undefine __WRL_WINRT_STRICT__ or remove CoCreatableClassWithFactoryEx macro");
#define CoCreatableClassWithFactory(className, factory) \
    static_assert(false, "Activation of COM components. Please make sure that that you either undefine __WRL_WINRT_STRICT__ or remove CoCreatableClassWithFactory macro");
#define CoCreatableClass(className) \
    static_assert(false, "Activation of COM components. Please make sure that that you either undefine __WRL_WINRT_STRICT__ or remove CoCreatableClass macro");
#endif

namespace Details
{
// Forwarding declaration of DefaultModule<>
template <ModuleType moduleType>
class DefaultModule;
} // namespace Details

// Forwarding declaraton of Module<>
template <ModuleType moduleType, typename ModuleT = Details::DefaultModule<moduleType>>
class Module;

template <typename ModuleT>
class Module<InProc, ModuleT> :
    public Details::ModuleBase
{
private:
    void VerifyEntries() noexcept
    {
        // Walk the linker generated list of pointers to CreatorMap for WinRT objects
        for (const Details::CreatorMap** entry = GetMidEntryPointer() + 1; entry < GetLastEntryPointer(); entry++)
        {
            if (*entry == nullptr)
            {
                continue;
            }

            const wchar_t* name = ((*entry)->activationId.getRuntimeName)();
            (void)(name);
            // Make sure that runtime class name is not nullptr and it has no empty string
            __WRL_ASSERT__(name != nullptr && ::wcslen(name) != 0);
        }

#ifdef _DEBUG
        Details::CheckForDuplicateEntries((GetFirstEntryPointer() + 1), GetMidEntryPointer(),
            [](const Details::CreatorMap* entry, const Details::CreatorMap* entry2) -> void {
                __WRL_ASSERT__(entry->activationId.clsid != entry2->activationId.clsid && "Duplicate CLSID!");
            }
        );

        Details::CheckForDuplicateEntries((GetMidEntryPointer() + 1), GetLastEntryPointer(),
            [](const Details::CreatorMap* entry, const Details::CreatorMap* entry2) -> void {
                __WRL_ASSERT__(::wcscmp((entry->activationId.getRuntimeName)(), (entry2->activationId.getRuntimeName)()) != 0 && "Duplicate runtime class name!");
            }
        );
#endif
    }

// If static initialization is not available there is no need
// to keep isInitialized and StaticInitialize
#ifndef __WRL_DISABLE_STATIC_INITIALIZE__
    static bool StaticInitialize()
    {
        ModuleT::Create();
        return true;
    }
    static bool isInitialized;
#endif
protected:
    Module()
    {
#ifdef _DEBUG
        VerifyEntries();
#endif
    }

public:
    virtual ~Module() noexcept
    {
        Details::TerminateMap(this, nullptr, true);
#ifndef __WRL_DISABLE_STATIC_INITIALIZE__
        // Needs to be changed to avoid compiler optimization
        isInitialized = false;
#endif
    }

    static ModuleT& Create() noexcept
    {
        // mingw-w64 has no DllMain thread attach penalty for function statics
        // (the SDK uses InitOnceExecuteOnce + StaticStorage to avoid it).
        static ModuleT moduleSingleton;
        return moduleSingleton;
    }

    static ModuleT& GetModule() noexcept
    {
        return Create();
    }

    HRESULT GetActivationFactory(HSTRING activatibleClassId, IActivationFactory** ppIFactory, const wchar_t* serverName = nullptr) noexcept
    {
        return Details::GetActivationFactory<InProc>(this, serverName, activatibleClassId, ppIFactory);
    }

    HRESULT GetClassObject(REFCLSID clsid, REFIID riid, void** ppv, const wchar_t* serverName = nullptr) noexcept
    {
        return Details::GetClassObject<InProc>(this, serverName, clsid, riid, ppv);
    }

    bool Terminate(const wchar_t* serverName = nullptr, bool forceTerminate = false) noexcept
    {
        return Details::TerminateMap(this, serverName, forceTerminate);
    }

    // Number of active objects in the module
    STDMETHOD_(unsigned long, IncrementObjectCount)()
    {
        return static_cast<unsigned long>(::InterlockedIncrement(reinterpret_cast<volatile LONG*>(&objectCount_)));
    }

    STDMETHOD_(unsigned long, DecrementObjectCount)()
    {
        return static_cast<unsigned long>(::InterlockedDecrement(reinterpret_cast<volatile LONG*>(&objectCount_)));
    }

    // InProc module doesn't implement any registration API's
    STDMETHOD(RegisterWinRTObject)(const wchar_t*, const wchar_t**, RO_REGISTRATION_COOKIE*, unsigned int)
    {
        Details::RoOriginateError(E_NOTIMPL, nullptr);
        return E_NOTIMPL;
    }

    STDMETHOD(UnregisterWinRTObject)(const wchar_t*, RO_REGISTRATION_COOKIE)
    {
        Details::RoOriginateError(E_NOTIMPL, nullptr);
        return E_NOTIMPL;
    }

    STDMETHOD(RegisterCOMObject)(const wchar_t*, IID*, IClassFactory**, DWORD*, unsigned int)
    {
        Details::RoOriginateError(E_NOTIMPL, nullptr);
        return E_NOTIMPL;
    }

    STDMETHOD(UnregisterCOMObject)(const wchar_t*, DWORD*, unsigned int)
    {
        Details::RoOriginateError(E_NOTIMPL, nullptr);
        return E_NOTIMPL;
    }
};

// Trigger static initialization unless explicitly disabled. Static initialization can be disabled
// to avoid dependencies on C++ constructors when using a DllEntry point other than the Crt startup
// entry point.
#ifndef __WRL_DISABLE_STATIC_INITIALIZE__
template <typename ModuleT>
bool Module<InProc, ModuleT>::isInitialized = Module<InProc, ModuleT>::StaticInitialize();
#endif

template <typename ModuleT>
class Module<InProcDisableCaching, ModuleT> :
    public Module<InProc, ModuleT>
{
public:
    HRESULT GetActivationFactory(HSTRING activatibleClassId, IActivationFactory** ppIFactory, const wchar_t* serverName = nullptr) noexcept
    {
        return Details::GetActivationFactory<InProcDisableCaching>(this, serverName, activatibleClassId, ppIFactory);
    }

    HRESULT GetClassObject(REFCLSID clsid, REFIID riid, void** ppv, const wchar_t* serverName = nullptr) noexcept
    {
        return Details::GetClassObject<InProcDisableCaching>(this, serverName, clsid, riid, ppv);
    }
};

namespace Details
{

template <typename ModuleT>
class OutOfProcModuleBase :
    public Module<InProc, ModuleT>
{
private:
    // GetObjectCount returns always zero for out of proc servers
    // This method is called in TerminateMap helper function
    STDMETHOD_(unsigned long, GetObjectCount)() const
    {
        return 0;
    }

protected:
    // Generic notification handler interface required to fire
    // when the last object on the module was released
    class ReleaseNotifier
    {
    public:
        ReleaseNotifier(bool release) noexcept : release_(release)
        {
        }
        virtual ~ReleaseNotifier() noexcept
        {
        }
        void Release() noexcept
        {
            if (release_)
            {
                delete this;
            }
        }
        virtual void Invoke() noexcept = 0;
    private:
        bool release_;
    };
    // Specialization for notify handler made with lambda, functors or pointer to function
    template <typename T>
    class GenericReleaseNotifier : public ReleaseNotifier
    {
    public:
        GenericReleaseNotifier(T callback, bool release) noexcept : ReleaseNotifier(release), callback_(callback)
        {
        }
        void Invoke() noexcept override
        {
            callback_();
        }
    protected:
        T callback_;
    };
    // Specialization for notify handler made with pointer to the method
    template <typename T>
    class MethodReleaseNotifier : public ReleaseNotifier
    {
    public:
        MethodReleaseNotifier(T* object, void (T::* method)(), bool release) noexcept :
            ReleaseNotifier(release), object_(object), method_(method)
        {
        }
        void Invoke() noexcept override
        {
            (object_->*method_)();
        }
    protected:
        T* object_;
        void (T::* method_)();
    };

    ReleaseNotifier* releaseNotifier_;

    OutOfProcModuleBase() noexcept : releaseNotifier_(nullptr)
    {
    }

    // The initialization functions provided to register notify handler
    // when the module is created with new/delete
    template <typename T>
    HRESULT Initialize(T callback) noexcept
    {
        // Module was already initialized
        __WRL_ASSERT__(releaseNotifier_ == nullptr);

        releaseNotifier_ = new (std::nothrow) GenericReleaseNotifier<T>(callback, true);
        if (releaseNotifier_ == nullptr)
        {
            return E_OUTOFMEMORY;
        }
        return S_OK;
    }

    template <typename T>
    HRESULT Initialize(T* object, void (T::* method)()) noexcept
    {
        // Module was already initialized
        __WRL_ASSERT__(releaseNotifier_ == nullptr);

        releaseNotifier_ = new (std::nothrow) MethodReleaseNotifier<T>(object, method, true);
        if (releaseNotifier_ == nullptr)
        {
            return E_OUTOFMEMORY;
        }
        return S_OK;
    }

public:
    virtual ~OutOfProcModuleBase() noexcept
    {
        if (releaseNotifier_ != nullptr)
        {
            releaseNotifier_->Release();
            releaseNotifier_ = nullptr;
        }
    }

    static ModuleT& Create() noexcept
    {
        static ModuleT moduleSingleton;
        return moduleSingleton;
    }

    template <typename T>
    static ModuleT& Create(T callback) noexcept
    {
        auto& moduleRef = Create();

        // Module was already initialized
        __WRL_ASSERT__(moduleRef.releaseNotifier_ == nullptr);

        if (moduleRef.releaseNotifier_ == nullptr)
        {
            // Not owned by the module (release == false); lives until process exit.
            alignas(GenericReleaseNotifier<T>) static unsigned char storage[sizeof(GenericReleaseNotifier<T>)];
            moduleRef.releaseNotifier_ = new (storage) GenericReleaseNotifier<T>(callback, false);
        }
        return moduleRef;
    }

    template <typename T>
    static ModuleT& Create(T* object, void (T::* method)()) noexcept
    {
        auto& moduleRef = Create();

        // Module was already created initialized
        __WRL_ASSERT__(moduleRef.releaseNotifier_ == nullptr);

        if (moduleRef.releaseNotifier_ == nullptr)
        {
            // Not owned by the module (release == false); lives until process exit.
            alignas(MethodReleaseNotifier<T>) static unsigned char storage[sizeof(MethodReleaseNotifier<T>)];
            moduleRef.releaseNotifier_ = new (storage) MethodReleaseNotifier<T>(object, method, false);
        }
        return moduleRef;
    }

    static ModuleT& GetModule() noexcept
    {
        auto& moduleRef = Create();
        return moduleRef;
    }
};

} // Details

template <typename ModuleT>
class Module<OutOfProc, ModuleT> :
    public Details::OutOfProcModuleBase<ModuleT>
{
    using Super = Details::OutOfProcModuleBase<ModuleT>;
public:
#ifndef __WRL_WINRT_STRICT__
    STDMETHOD(RegisterCOMObject)(const wchar_t* serverName, IID* clsids, IClassFactory** factories, DWORD* cookies, unsigned int count)
    {
        return Details::RegisterCOMObject<REGCLS_MULTIPLEUSE>(serverName, clsids, factories, cookies, count);
    }

    STDMETHOD(UnregisterCOMObject)(const wchar_t*, DWORD* cookies, unsigned int count)
    {
        HRESULT hr = S_OK;

        for (unsigned int i = 0 ; i < count && SUCCEEDED(hr); i++)
        {
            if (cookies[i] != 0)
            {
                hr = ::CoRevokeClassObject(cookies[i]);
                if (SUCCEEDED(hr))
                {
                    cookies[i] = 0;
                }
            }
        }

        return hr;
    }
#else
    STDMETHOD(RegisterCOMObject)(const wchar_t*, IID*, IClassFactory**, DWORD*, unsigned int)
    {
        __WRL_ASSERT__(false && "COM components found. Please make sure that that you either undefine __WRL_WINRT_STRICT__ or remove COM components");
        return S_OK;
    }

    STDMETHOD(UnregisterCOMObject)(const wchar_t*, DWORD*, unsigned int)
    {
        __WRL_ASSERT__(false && "COM components found. Please make sure that that you either undefine __WRL_WINRT_STRICT__ or remove COM components");
        return S_OK;
    }
#endif  // __WRL_WINRT_STRICT__

#ifndef __WRL_CLASSIC_COM_STRICT__
    STDMETHOD(RegisterWinRTObject)(const wchar_t* serverName, const wchar_t** activatableClassIds, RO_REGISTRATION_COOKIE* cookie, unsigned int count)
    {
        return Details::RegisterWinRTObject<OutOfProc>(serverName, activatableClassIds, cookie, count);
    }

    STDMETHOD(UnregisterWinRTObject)(const wchar_t*, RO_REGISTRATION_COOKIE cookie)
    {
        ::RoRevokeActivationFactories(cookie);
        return S_OK;
    }
#else
    STDMETHOD(RegisterWinRTObject)(const wchar_t*, const wchar_t**, RO_REGISTRATION_COOKIE*, unsigned int)
    {
        __WRL_ASSERT__(false && "WinRT components found. Please make sure that that you either undefine __WRL_CLASSIC_COM_STRICT__ or remove WinRT components");
        return S_OK;
    }

    STDMETHOD(UnregisterWinRTObject)(const wchar_t*, RO_REGISTRATION_COOKIE)
    {
        __WRL_ASSERT__(false && "WinRT components found. Please make sure that that you either undefine __WRL_CLASSIC_COM_STRICT__ or remove WinRT components");
        return S_OK;
    }
#endif // __WRL_CLASSIC_COM_STRICT__

    HRESULT RegisterObjects(const wchar_t* serverName = nullptr) noexcept
    {
        return Details::RegisterObjects<OutOfProc>(this, serverName);
    }

    HRESULT UnregisterObjects(const wchar_t* serverName = nullptr) noexcept
    {
       return Details::UnregisterObjects(this, serverName);
    }

    STDMETHOD_(unsigned long, IncrementObjectCount)()
    {
        return ::CoAddRefServerProcess();
    }

    STDMETHOD_(unsigned long, DecrementObjectCount)()
    {
        auto ref = ::CoReleaseServerProcess();
        if (ref == 0 && Super::releaseNotifier_)
        {
            Super::releaseNotifier_->Invoke();
        }

        return ref;
    }
};

template <typename ModuleT>
class Module<OutOfProcDisableCaching, ModuleT> :
    public Module<OutOfProc, ModuleT>
{
public:
    HRESULT GetActivationFactory(HSTRING activatibleClassId, IActivationFactory** ppIFactory, const wchar_t* serverName = nullptr) noexcept
    {
        // Those methods are called in context of InProc always
        return Details::GetActivationFactory<InProcDisableCaching>(this, serverName, activatibleClassId, ppIFactory);
    }

    HRESULT GetClassObject(REFCLSID clsid, REFIID riid, void** ppv, const wchar_t* serverName = nullptr) noexcept
    {
        // Those methods are called in context of InProc always
        return Details::GetClassObject<InProcDisableCaching>(this, serverName, clsid, riid, ppv);
    }

    HRESULT RegisterObjects(const wchar_t* serverName = nullptr) noexcept
    {
        return Details::RegisterObjects<OutOfProcDisableCaching>(this, serverName);
    }

#ifndef __WRL_CLASSIC_COM_STRICT__
    STDMETHOD(RegisterWinRTObject)(const wchar_t* serverName, const wchar_t** activatableClassIds, RO_REGISTRATION_COOKIE* cookies, unsigned int count)
    {
        return Details::RegisterWinRTObject<OutOfProcDisableCaching>(serverName, activatableClassIds, cookies, count);
    }
#endif
};

namespace Details
{
template <ModuleType moduleType>
class DefaultModule :
    public Module<moduleType, DefaultModule<moduleType>>
{
};
} // namespace Details

}} // namespace Microsoft::WRL

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif // _WRL_MODULE_H_
