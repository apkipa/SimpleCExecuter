#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "libtcc.h"

#include "public.h"

#pragma comment(lib, "libtcc.lib")
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define C_EXECUTER_WINDOW_CLASS_NAME L"SimpleCExecuterClass"
#define C_EXECUTER_WINDOW_NAME L"Simple C Executer"

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

ATOM My_RegisterClass(HINSTANCE hInstance) {
	WNDCLASS WndClass;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hInstance = hInstance;
	WndClass.lpfnWndProc = WndProc;
	WndClass.lpszClassName = C_EXECUTER_WINDOW_CLASS_NAME;
	WndClass.lpszMenuName = NULL;
	WndClass.style = 0;
	return RegisterClass(&WndClass);
}

HWND My_CreateWindow(HINSTANCE hInstance) {
	return CreateWindow(
		C_EXECUTER_WINDOW_CLASS_NAME,
		C_EXECUTER_WINDOW_NAME,
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		NULL,
		NULL,
		hInstance,
		NULL
	);
}

int My_CycleMessage(void) {
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}

static void ExecuteErrorCallback(void *data, const char *str) {
	MessageBoxA((HWND)data, str, NULL, MB_ICONERROR);
}

bool RunSeparatedCode(wchar_t strSharedMemoryName[128], int *nRet) {
	pSeparateMode_SharedData pShared;
	//PtrFuncType_main UserSpace_main;
	UserSpace_EntryPoints eps;
	bool bConsoleAutoPause;
	HANDLE hSharedMemory;
	bool bUseConsole;
	TCCState *pState;
	HWND hwnd;

	SetLastError(ERROR_SUCCESS);
	hSharedMemory = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 1, strSharedMemoryName);
	if (GetLastError() != ERROR_ALREADY_EXISTS) {
		//Something went wrong. Just return.
		if (hSharedMemory != NULL)
			CloseHandle(hSharedMemory);
		return false;
	}

	pShared = (pSeparateMode_SharedData)MapViewOfFile(hSharedMemory, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (pShared == NULL) {
		CloseHandle(hSharedMemory);
		return false;
	}

	pShared->bReceived = true;
	hwnd = pShared->hwnd;
	bUseConsole = pShared->bUseConsole;
	bConsoleAutoPause = pShared->bConsoleAutoPause;

	pState = tcc_new();
	if (pState == NULL) {
		UnmapViewOfFile(pShared);
		CloseHandle(hSharedMemory);
		return false;
	}
	tcc_set_error_func(pState, hwnd, ExecuteErrorCallback);
	tcc_add_sysinclude_path(pState, "tcc/include");
	tcc_add_sysinclude_path(pState, "tcc/include/winapi");
	tcc_add_library_path(pState, "tcc/lib");
	TccLibFuncInject_Early(pState);

	tcc_set_output_type(pState, TCC_OUTPUT_MEMORY);
	if (tcc_compile_string(pState, pShared->code) == -1 || tcc_relocate(pState, TCC_RELOCATE_AUTO) < 0) {
		tcc_delete(pState);
		UnmapViewOfFile(pShared);
		CloseHandle(hSharedMemory);
		return false;
	}

	UnmapViewOfFile(pShared);
	CloseHandle(hSharedMemory);

	TccLibFuncInject_Late(pState);

	if (bUseConsole) {
		eps.UserSpace_main = (PtrFuncType_main)tcc_get_symbol(pState, "main");
		if (eps.UserSpace_main == NULL) {
			ExecuteErrorCallback(hwnd, "Cannot locate entry point \"main\" in program \"main.exe\".");
			tcc_delete(pState);
			return false;
		}

		InitStdConsole();

		__try {
			*nRet = eps.UserSpace_main(1, (char*[]) { "main.exe" });
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			ExecuteErrorCallback(hwnd, "A fatal error occurred during program execution. The program has been stopped.");

			UninitStdConsole();

			tcc_delete(pState);
			return false;
		}

		if (bConsoleAutoPause)
			system("pause");

		UninitStdConsole();
	}
	else {
		eps.UserSpace_WinMain = (PtrFuncType_WinMain)tcc_get_symbol(pState, "_WinMain@16");
		if (eps.UserSpace_WinMain == NULL) {
			ExecuteErrorCallback(hwnd, "Cannot locate entry point \"WinMain\" in program \"main.exe\".");
			tcc_delete(pState);
			return false;
		}

		__try {
			*nRet = eps.UserSpace_WinMain(GetModuleHandle(NULL), NULL, (char[1]) { "" }, SW_NORMAL);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			ExecuteErrorCallback(hwnd, "A fatal error occurred during program execution. The program has been stopped.");

			tcc_delete(pState);
			return false;
		}
	}

	/*
	UserSpace_main = (PtrFuncType_main)tcc_get_symbol(pState, "main");
	if (UserSpace_main == NULL) {
		ExecuteErrorCallback(hwnd, "Cannot locate entry point \"main\" in program \"main.exe\".");
		tcc_delete(pState);
		return false;
	}

	UnmapViewOfFile(pShared);
	CloseHandle(hSharedMemory);

	AllocConsole();
	freopen("CONIN$", "r+t", stdin);
	freopen("CONOUT$", "w+t", stdout);
	freopen("CONOUT$", "w+t", stderr);

	__try {
		*nRet = UserSpace_main(1, (char*[]) { "main.exe" });
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		ExecuteErrorCallback(hwnd, "A fatal error occured during program execution. The program has been stopped.");

		fclose(stderr);
		fclose(stdout);
		fclose(stdin);
		FreeConsole();

		tcc_delete(pState);
		return false;
	}

	if (bConsoleAutoPause)
		system("pause");

	fclose(stderr);
	fclose(stdout);
	fclose(stdin);
	FreeConsole();
	*/

	tcc_delete(pState);
	return true;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
	if (wcsncmp(lpCmdLine, L"execute ", wcslen(L"execute ")) == 0) {
		//This instance is being executed as a code executer instead of an editor
		wchar_t strSharedMemoryName[128];
		int nRet;

		strSharedMemoryName[0] = '\0';
		swscanf(lpCmdLine + wcslen(L"execute "), L"%s", strSharedMemoryName);

		return RunSeparatedCode(strSharedMemoryName, &nRet) ? nRet : 1;
	}

	if (LoadLibrary(L"SciLexer.dll") == NULL) {
		FatalAppExit(0, L"SciLexer.dll is not found and program cannot continue. Reinstalling program may solve this problem.");
		return 1;
	}

	My_RegisterClass(hInstance);
	My_CreateWindow(hInstance);
	return My_CycleMessage();
}