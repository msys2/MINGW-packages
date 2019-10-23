#include <windows.h>
#include <shlwapi.h>
#include <shellapi.h>

int main(int argc, char** argv) {
	char* root = argv[1];
	char** files = {root, NULL};
	SHFILEOPSTRUCT op;
	op.hwnd = NULL;
	op.wFunc = FO_DELETE;
	op.pFrom = files;
	op.pTo = NULL;
	op.fFlags = FOF_NOCONFIRMATION|FOF_SILENT;
	while(PathFileExists(root) && SHFileOperation(&op))
		Sleep(1000);
	return 0;
}
