#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <stdbool.h>
#include <stdio.h>

#include "Scintilla.h"
#include "SciLexer.h"
#include "libtcc.h"

#include "public.h"

#pragma comment(lib, "Imm32.lib")

#define CODE_EDITOR_ID 101
#define CONSOLE_ID	102	//NOTE: Console may be realized with AllocConsole()

#define OPERATION_BUTTON_WIDTH 60
#define OPERATION_BAR_HEIGHT 30

#define OPERATION_BUTTON_RUN 1
#define OPERATION_BUTTON_INFO 3
#define OPERATION_BUTTON_LAST 3

#define SYSMENU_CHECK_USE_CONSOLE (1 << 4)
#define SYSMENU_CHECK_SEPARATE_MODE (2 << 4)
#define SYSMENU_CHECK_CONSOLE_AUTO_PAUSE (3 << 4)
#define SYSMENU_RADIO_CODEPAGE_UTF8 (101 << 4)
#define SYSMENU_RADIO_CODEPAGE_SC_GBK (102 << 4)

//Text area - begin
#define TEXT_LANGUAGE_ID TEXT_LANGUAGE_ID_ENGLISH	//Change this if you want another language

#if (TEXT_LANGUAGE_ID == TEXT_LANGUAGE_ID_CHINESE)
#define OPERATION_BUTTON_RUN_TEXT L"运行"
#define OPERATION_BUTTON_INFO_TEXT L"关于"

#define SYSMENU_CHECK_USE_CONSOLE_TEXT L"模拟控制台环境 (取消勾选以模拟 GUI 环境)"
#define SYSMENU_CHECK_SEPARATE_MODE_TEXT L"分离模式 (在一个新建实例中运行代码，略微降低性能)"
#define SYSMENU_CHECK_CONSOLE_AUTO_PAUSE_TEXT L"控制台程序结束后自动暂停"
#define SYSMENU_RADIO_CODEPAGE_TEXT L"代码页"
#define SYSMENU_RADIO_CODEPAGE_UTF8_TEXT L"UTF-8"
#define SYSMENU_RADIO_CODEPAGE_SC_GBK_TEXT L"简体中文 GBK (936)"

#define ABOUT_TEXT L"关于"
#define PROGRAM_INFO_TEXT L"Simple C Executer\n作者: apkipa\n本程序使用的支持库:\nlibtcc 0.9.2.7\nScintilla 4.1.0"

#define STABILITY_WARNING_TITLE_TEXT L"稳定性警告"
#define STABILITY_WARNING_TEXT L"取消勾选该选项可能会导致代码的潜在漏洞影响本程序，进而导致严重的损失!\n仍然要这么做吗?"
#elif (TEXT_LANGUAGE_ID == TEXT_LANGUAGE_ID_ENGLISH)
#define OPERATION_BUTTON_RUN_TEXT L"Execute"
#define OPERATION_BUTTON_INFO_TEXT L"Info"

#define SYSMENU_CHECK_USE_CONSOLE_TEXT L"Emulate console environment (Untick to emulate GUI environment)"
#define SYSMENU_CHECK_SEPARATE_MODE_TEXT L"Separate mode (Run code in a new instance, slightly reduce performance)"
#define SYSMENU_CHECK_CONSOLE_AUTO_PAUSE_TEXT L"Auto pause after console program ended"
#define SYSMENU_RADIO_CODEPAGE_TEXT L"Code page"
#define SYSMENU_RADIO_CODEPAGE_UTF8_TEXT L"UTF-8"
#define SYSMENU_RADIO_CODEPAGE_SC_GBK_TEXT L"Simplified Chinese GBK (936)"

#define ABOUT_TEXT L"About"
#define PROGRAM_INFO_TEXT L"Simple C Executer\nAuthor: apkipa\nLibraries used:\nlibtcc 0.9.2.7\nScintilla 4.1.0"

#define STABILITY_WARNING_TITLE_TEXT L"Stability Warning"
#define STABILITY_WARNING_TEXT L"Unticking this option may cause the program affected by unstable code and lead to fatal errors!\nStill continue?"
#else
#error Unknown language.
#endif
//Text area - end

#define SYSMENU_RADIO_CODEPAGE_BEGIN SYSMENU_RADIO_CODEPAGE_UTF8
#define SYSMENU_RADIO_CODEPAGE_END SYSMENU_RADIO_CODEPAGE_SC_GBK

#define SCE_C_BEGIN SCE_C_DEFAULT
#define SCE_C_END SCE_C_ESCAPESEQUENCE + 10

HMENU menuCodePagePopup;

/*
#include <stdlib.h>
#include <stdio.h>

int main(void) {
	char c;

	printf("Enter a string: ");

	while ((c = getchar()) != '\n') {
		putchar(c);
	}
	putchar('\n');
	putc('\n', stdout);

	//system("pause");
}
*/

static void ExecuteErrorCallback(void *data, const char *str) {
	MessageBoxA((HWND)data, str, NULL, MB_ICONERROR);
}

bool RunStringCode_Direct(HWND hwnd, char *str, bool bUseConsole, bool bConsoleAutoPause) {
	PtrFuncType_main UserSpace_main;
	TCCState *pState;
	//int nRet;

	if (bUseConsole) {
		pState = tcc_new();
		if (pState == NULL)
			return false;

		tcc_set_error_func(pState, hwnd, ExecuteErrorCallback);
		tcc_add_sysinclude_path(pState, "tcc/include");
		tcc_add_sysinclude_path(pState, "tcc/include/winapi");
		tcc_add_library_path(pState, "tcc/lib");
		TccLibFuncInject_Early(pState);

		tcc_set_output_type(pState, TCC_OUTPUT_MEMORY);
		if (tcc_compile_string(pState, str) == -1 || tcc_relocate(pState, TCC_RELOCATE_AUTO) < 0) {
			tcc_delete(pState);
			return false;
		}

		TccLibFuncInject_Late(pState);

		UserSpace_main = (PtrFuncType_main)tcc_get_symbol(pState, "main");
		if (UserSpace_main == NULL) {
			ExecuteErrorCallback(hwnd, "Cannot locate entry point \"main\" in program \"main.exe\".");
			tcc_delete(pState);
			return false;
		}

		AllocConsole();
		freopen("CONIN$", "r+t", stdin);
		freopen("CONOUT$", "w+t", stdout);
		freopen("CONOUT$", "w+t", stderr);

		__try {
			UserSpace_main(1, (char*[]) { "main.exe" });
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

		tcc_delete(pState);
	}
	else {

	}

	return true;
}

bool RunStringCode_Separated(HWND hwnd, char *str, bool bUseConsole, bool bConsoleAutoPause) {
	wchar_t strCommandLine[MAX_PATH * 2];
	pSeparateMode_SharedData pShared;
	wchar_t strSharedMemoryName[128];
	static long long nCounter = 0;
	size_t nTotalSize, nStrLen;
	HANDLE hSharedMemory;

	nStrLen = strlen(str);
	nTotalSize = sizeof(SeparateMode_SharedData) + nStrLen + 1;
	swprintf(strSharedMemoryName, 128, L"Local\\SimpleCExecuter_SharedMemoryId_Counter%lld", nCounter++);

	SetLastError(ERROR_SUCCESS);
	hSharedMemory = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, nTotalSize, strSharedMemoryName);
	if (hSharedMemory == NULL)
		return false;
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		CloseHandle(hSharedMemory);
		return false;
	}

	pShared = (pSeparateMode_SharedData)MapViewOfFile(hSharedMemory, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (pShared == NULL) {
		CloseHandle(hSharedMemory);
		return false;
	}

	pShared->nStructureSize = nTotalSize;
	pShared->hwnd = hwnd;
	pShared->bUseConsole = bUseConsole;
	pShared->bSeparated = true;
	pShared->bConsoleAutoPause = bConsoleAutoPause;
	pShared->bReceived = false;
	strcpy(pShared->code, str);

	GetModuleFileName(NULL, strCommandLine, MAX_PATH);
	wcscat(strCommandLine, L" execute ");
	wcscat(strCommandLine, strSharedMemoryName);

	PROCESS_INFORMATION pi = { 0 };
	STARTUPINFO si = { 0 };
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_FORCEOFFFEEDBACK;
	CreateProcess(NULL, strCommandLine, NULL, NULL, false, 0, NULL, NULL, &si, &pi);

	while (!pShared->bReceived)
		Sleep(10);

	UnmapViewOfFile(pShared);
	CloseHandle(hSharedMemory);

	return true;
}

bool RunStringCode(HWND hwnd, char *str, bool bUseConsole, bool bSeparated, bool bConsoleAutoPause) {
	return (bSeparated ? RunStringCode_Separated : RunStringCode_Direct)(hwnd, str, bUseConsole, bConsoleAutoPause);
}

void AddOperationBarButton(HWND hwnd, HINSTANCE hInstance, int id) {
	static wchar_t *btnNameList[] = {
		[OPERATION_BUTTON_RUN] = OPERATION_BUTTON_RUN_TEXT,
		[OPERATION_BUTTON_INFO] = OPERATION_BUTTON_INFO_TEXT
	};
	RECT rtClient;
	GetClientRect(hwnd, &rtClient);
	CreateWindow(
		L"Button",
		btnNameList[id],
		WS_CHILD | WS_VISIBLE,
		(id - 1) * OPERATION_BUTTON_WIDTH, rtClient.bottom - OPERATION_BAR_HEIGHT,
		OPERATION_BUTTON_WIDTH, OPERATION_BAR_HEIGHT,
		hwnd,
		(HMENU)id,
		hInstance,
		NULL
	);
}

void RelocateOperationBarButton(HWND hwnd) {
	RECT rtClient;
	GetClientRect(hwnd, &rtClient);
	for (int i = 0; i <= OPERATION_BUTTON_LAST; i++)
		SetWindowPos(GetDlgItem(hwnd, i), NULL, (i - 1) * OPERATION_BUTTON_WIDTH, rtClient.bottom - OPERATION_BAR_HEIGHT, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void WndProc_Create(HWND hwnd, LPCREATESTRUCT lpCreateStruct) {
	HMENU menuSystem;
	HWND hwEditor;

	hwEditor = CreateWindow(L"Scintilla", NULL, WS_CHILD | WS_VISIBLE, 0, 0, 100, 100, hwnd, (HMENU)CODE_EDITOR_ID, lpCreateStruct->hInstance, NULL);
	SendMessage(hwEditor, SCI_SETLEXER, SCLEX_CPP, 0);
	for (int i = SCE_C_BEGIN; i <= SCE_C_END; i++) {
		SendMessage(hwEditor, SCI_STYLESETFONT, i, (LPARAM)"FixedSys");
	}
	SendMessage(hwEditor, SCI_SETTABWIDTH, 4, 0);
	//SendMessage(hwEditor, SCI_STYLESETFONT, SCE_C_DEFAULT, (LPARAM)"Courier New");
	SendMessage(hwEditor, SCI_STYLESETFORE, SCE_C_WORD, RGB(0, 0, 255));
	//SendMessage(hwEditor, SCI_STYLESETFORE, SCE_C_PREPROCESSOR, RGB(0, 0, 255));

	SendMessage(
		hwEditor,
		SCI_SETKEYWORDS,
		0,
		(LPARAM)
		"int short float double signed unsigned char long void bool _Bool const static register auto wchar_t "
		"int8_t int16_t int32_t int64_t uint8_t uint16_t uint32_t uint64_t intmax_t uintmax_t intptr_t uintptr_t "
		"int_fast8_t int_fast16_t int_fast32_t int_fast64_t uint_fast8_t uint_fast16_t uint_fast32_t uint_fast64_t "
		"int_least8_t int_least16_t int_least32_t int_least64_t uint_least8_t uint_least16_t uint_least32_t uint_least64_t "
		"size_t ssize_t ptrdiff_t clock_t va_list struct enum union"
	);

	SendMessage(hwEditor, SCI_SETMARGINTYPEN, 0, SC_MARGIN_NUMBER);
	SendMessage(hwEditor, SCI_SETMARGINWIDTHN, 0, SendMessage(hwEditor, SCI_TEXTWIDTH, STYLE_LINENUMBER, (LPARAM)"_1"));
	//SendMessage(hwEditor, SCI_SETMARGINMASKN, 0, SC_MARK_ARROWS);
	//SendMessage(hwEditor, SCI_SETCODEPAGE, 936, 0);

	AddOperationBarButton(hwnd, lpCreateStruct->hInstance, OPERATION_BUTTON_RUN);
	AddOperationBarButton(hwnd, lpCreateStruct->hInstance, OPERATION_BUTTON_INFO);

	menuCodePagePopup = CreatePopupMenu();
	AppendMenu(menuCodePagePopup, MF_STRING | MF_CHECKED | MFT_RADIOCHECK, SYSMENU_RADIO_CODEPAGE_UTF8, SYSMENU_RADIO_CODEPAGE_UTF8_TEXT);
	AppendMenu(menuCodePagePopup, MF_STRING, SYSMENU_RADIO_CODEPAGE_SC_GBK, SYSMENU_RADIO_CODEPAGE_SC_GBK_TEXT);
	menuSystem = GetSystemMenu(hwnd, false);
	AppendMenu(menuSystem, MF_SEPARATOR, 0, NULL);
	AppendMenu(menuSystem, MF_STRING | MF_CHECKED, SYSMENU_CHECK_USE_CONSOLE, SYSMENU_CHECK_USE_CONSOLE_TEXT);
	AppendMenu(menuSystem, MF_STRING | MF_CHECKED, SYSMENU_CHECK_SEPARATE_MODE, SYSMENU_CHECK_SEPARATE_MODE_TEXT);
	AppendMenu(menuSystem, MF_STRING | MF_CHECKED, SYSMENU_CHECK_CONSOLE_AUTO_PAUSE, SYSMENU_CHECK_CONSOLE_AUTO_PAUSE_TEXT);
	AppendMenu(menuSystem, MF_STRING | MF_POPUP, (UINT_PTR)menuCodePagePopup, SYSMENU_RADIO_CODEPAGE_TEXT);

	SetFocus(hwEditor);
}

void WndProc_Destroy(HWND hwnd) {
	PostQuitMessage(0);
}

void WndProc_Command_OperationButton_Run(HWND hwnd, int nControlNotificationCode, HWND hwControl) {
	bool bUseConsole, bSeparated, bConsoleAutoPause;
	MENUITEMINFO mii;
	HMENU menuSystem;
	HWND hwEditor;
	size_t nSize;
	char *str;

	hwEditor = GetDlgItem(hwnd, CODE_EDITOR_ID);
	menuSystem = GetSystemMenu(hwnd, false);

	mii.cbSize = sizeof(MENUITEMINFO);
	mii.fMask = MIIM_STATE;
	GetMenuItemInfo(menuSystem, SYSMENU_CHECK_USE_CONSOLE, false, &mii);
	bUseConsole = (bool)(mii.fState & MFS_CHECKED);
	GetMenuItemInfo(menuSystem, SYSMENU_CHECK_SEPARATE_MODE, false, &mii);
	bSeparated = (bool)(mii.fState & MFS_CHECKED);
	GetMenuItemInfo(menuSystem, SYSMENU_CHECK_CONSOLE_AUTO_PAUSE, false, &mii);
	bConsoleAutoPause = (bool)(mii.fState & MFS_CHECKED);

	nSize = SendMessage(hwEditor, SCI_GETTEXT, 0, 0);

	str = malloc(nSize);

	SendMessage(hwEditor, SCI_GETTEXT, nSize, (LPARAM)str);

	RunStringCode(hwnd, str, bUseConsole, bSeparated, bConsoleAutoPause);

	free(str);
}

void WndProc_Command_OperationButton_Info(HWND hwnd, int nControlNotificationCode, HWND hwControl) {
	MessageBox(hwnd, PROGRAM_INFO_TEXT, ABOUT_TEXT, MB_ICONINFORMATION);
}

void WndProc_Command(HWND hwnd, int nControlId, int nControlNotificationCode, HWND hwControl) {
	switch (nControlId) {
	case OPERATION_BUTTON_RUN:
		WndProc_Command_OperationButton_Run(hwnd, nControlNotificationCode, hwControl);
		break;
	case OPERATION_BUTTON_INFO:
		WndProc_Command_OperationButton_Info(hwnd, nControlNotificationCode, hwControl);
		break;
	}
}

void WndProc_SystemCommand(HWND hwnd, int nCmdType, int xPos, int yPos) {
	MENUITEMINFO mii;
	HMENU menuSystem;

	menuSystem = GetSystemMenu(hwnd, false);

	switch (nCmdType) {
	case SYSMENU_CHECK_USE_CONSOLE:
	case SYSMENU_CHECK_SEPARATE_MODE:
	case SYSMENU_CHECK_CONSOLE_AUTO_PAUSE:
		mii.cbSize = sizeof(MENUITEMINFO);
		mii.fMask = MIIM_STATE;
		GetMenuItemInfo(menuSystem, nCmdType, false, &mii);
		if (nCmdType == SYSMENU_CHECK_SEPARATE_MODE && (mii.fState & MFS_CHECKED)) {
			if (
				MessageBox(
					hwnd,
					STABILITY_WARNING_TEXT,
					STABILITY_WARNING_TITLE_TEXT,
					MB_ICONWARNING | MB_YESNO
				) != IDYES) {
				break;
			}
		}
		mii.fState ^= MFS_CHECKED;
		SetMenuItemInfo(menuSystem, nCmdType, false, &mii);
		break;
	case SYSMENU_RADIO_CODEPAGE_UTF8:
	case SYSMENU_RADIO_CODEPAGE_SC_GBK:
		CheckMenuRadioItem(menuCodePagePopup, SYSMENU_RADIO_CODEPAGE_BEGIN, SYSMENU_RADIO_CODEPAGE_END, nCmdType, MF_BYCOMMAND);
		switch (nCmdType) {
		case SYSMENU_RADIO_CODEPAGE_UTF8:
			SendMessage(GetDlgItem(hwnd, CODE_EDITOR_ID), SCI_SETCODEPAGE, SC_CP_UTF8, 0);
			break;
		case SYSMENU_RADIO_CODEPAGE_SC_GBK:
			SendMessage(GetDlgItem(hwnd, CODE_EDITOR_ID), SCI_SETCODEPAGE, 936, 0);
			break;
		}
		break;
	}
}

void WndProc_Size(HWND hwnd, int nResizingRequested, int nClientWidth, int nClientHeight) {
	SetWindowPos(GetDlgItem(hwnd, CODE_EDITOR_ID), NULL, 0, 0, nClientWidth, nClientHeight - OPERATION_BAR_HEIGHT, SWP_NOMOVE | SWP_NOZORDER);
	RelocateOperationBarButton(hwnd);
}

void WndProc_Notify(HWND hwnd, int nIdCommon, NMHDR *pNmhdr) {
	int nLinenumOriginalWidth, nLinenumCalculatedWidth;
	char strBuffer[16];

	if (pNmhdr->idFrom == CODE_EDITOR_ID) {
		switch (pNmhdr->code) {
		case SCN_MODIFIED:
		case SCN_ZOOM:
			nLinenumOriginalWidth = SendMessage(pNmhdr->hwndFrom, SCI_GETMARGINWIDTHN, 0, 0);
			sprintf(strBuffer, "_%d", (int)SendMessage(pNmhdr->hwndFrom, SCI_GETLINECOUNT, 0, 0));
			nLinenumCalculatedWidth = SendMessage(pNmhdr->hwndFrom, SCI_TEXTWIDTH, STYLE_LINENUMBER, (LPARAM)strBuffer);
			if (nLinenumOriginalWidth != nLinenumCalculatedWidth)
				SendMessage(pNmhdr->hwndFrom, SCI_SETMARGINWIDTHN, 0, (LPARAM)nLinenumCalculatedWidth);
			break;
		}
	}
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_CREATE:
		WndProc_Create(hwnd, (LPCREATESTRUCT)lParam);
		break;
	case WM_DESTROY:
		WndProc_Destroy(hwnd);
		break;
	case WM_COMMAND:
		WndProc_Command(hwnd, LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
		break;
	case WM_SYSCOMMAND:
		WndProc_SystemCommand(hwnd, wParam & ~15, LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_SIZE:
		WndProc_Size(hwnd, (int)wParam, LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_NOTIFY:
		WndProc_Notify(hwnd, (int)wParam, (NMHDR*)lParam);
		break;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}