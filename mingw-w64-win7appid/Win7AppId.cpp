//
// Win7AppId
// Author: David Roe (didroe)
// The code's a bit rough and ready but it does the job
//
// Compile with VS 2010 express using command:
//     cl /EHsc /D _UNICODE Win7AppId.cpp /link ole32.lib

#include <windows.h>
#include <shobjidl.h>
#include <propkey.h>
#include <tchar.h>

#include <iostream>

using namespace std;

EXTERN_C const PROPERTYKEY DECLSPEC_SELECTANY PKEY_AppUserModel_ID =
 	  	     { { 0x9F4C2855, 0x9F79, 0x4B39,
	  	     { 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3, } }, 5 };

void die(string message) {
	cout << "Error: " << message.c_str() << endl;
	exit(1);
}

void doOrDie(HRESULT hr, string message) {
	if (!SUCCEEDED(hr)) {
		die(message);
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	doOrDie(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED),
			"Failed to initialise COM");

	IShellLink *link;
	doOrDie(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&link)),
			"Failed to create ShellLink object");
	
	IPersistFile* file; 
	doOrDie(link->QueryInterface(IID_PPV_ARGS(&file)),
			"Failed to obtain PersistFile interface"); 

	if (argc > 2) {
		doOrDie(file->Load(argv[1], STGM_READWRITE),
				"Failed to load shortcut file");
	} else {
		doOrDie(file->Load(argv[1], STGM_READ | STGM_SHARE_DENY_NONE),
				"Failed to load shortcut file");
	}
    
	IPropertyStore* store; 
	doOrDie(link->QueryInterface(IID_PPV_ARGS(&store)),
			"Failed to obtain PropertyStore interface"); 

	PROPVARIANT pv;
	doOrDie(store->GetValue(PKEY_AppUserModel_ID, &pv),
			"Failed to retrieve AppId");
	
	if (pv.vt != VT_EMPTY) {
		if (pv.vt != VT_LPWSTR) {
			cout << "Type: " << pv.vt << endl;
			die("Unexpected property value type");
		}

		wcout << "Current AppId: " << pv.pwszVal << endl;
	} else {
		cout << "No current AppId" << endl;
	}
	PropVariantClear(&pv);

	if (argc > 2) {
		wcout << "New AppId: " << argv[2] << endl;

		pv.vt = VT_LPWSTR;
		pv.pwszVal = argv[2];

		doOrDie(store->SetValue(PKEY_AppUserModel_ID, pv),
				"Failed to set AppId");

        // Not sure if we need to do this
        pv.pwszVal = NULL;
		PropVariantClear(&pv);

		doOrDie(store->Commit(),
				"Failed to commit AppId property");

		doOrDie(file->Save(NULL, TRUE),
				"Failed to save shortcut");
	}

	store->Release();
	file->Release();
	link->Release();	

	return 0;
}
