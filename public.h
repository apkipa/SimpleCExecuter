#pragma once

#define TEXT_LANGUAGE_ID_CHINESE 1
#define TEXT_LANGUAGE_ID_ENGLISH 2

#define OSFD_TYPE_OPEN 1
#define OSFD_TYPE_SAVE 2
#define OSFD_RET_SUCCEEDED 1
#define OSFD_RET_USERCANCELLED 0
#define OSFD_RET_FAILED -1

typedef int(*PtrFuncType_main)(int argc, char *argv[]);
typedef int(*PtrFuncType_wmain)(int argc, wchar_t *argv[]);
typedef int(WINAPI*PtrFuncType_WinMain)(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
typedef int(WINAPI*PtrFuncType_wWinMain)(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow);

typedef struct tagSeparateMode_SharedData {
	size_t nStructureSize;
	HWND hwnd;
	bool bUseConsole;
	bool bSeparated;
	bool bConsoleAutoPause;

	bool bReceived;

	char code[];
} SeparateMode_SharedData, *pSeparateMode_SharedData;

typedef struct tagUserSpace_EntryPoints {
	union {
		PtrFuncType_main UserSpace_main;
		PtrFuncType_wmain UserSpace_wmain;	//Not supported
		PtrFuncType_WinMain UserSpace_WinMain;
		PtrFuncType_wWinMain UserSpace_wWinMain;	//Not supported
	};
} UserSpace_EntryPoints, *pUserSpace_EntryPoints;

#define UserSpace_FileStructureSize 32
#define UserSpace_FileStructureReplace(_Stream) (		\
	UserSpace__imp___iob == NULL ? (FILE*)(_Stream) :	\
	(_Stream) == (*UserSpace__imp___iob) + 0 ? stdin :	\
	(_Stream) == (*UserSpace__imp___iob) + 1 ? stdout :	\
	(_Stream) == (*UserSpace__imp___iob) + 2 ? stderr :	\
	(FILE*)(_Stream)									\
)

typedef struct tagUserSpace_FILE {
	char placeholder[UserSpace_FileStructureSize];
} UserSpace_FILE, *pUserSpace_FILE;
static UserSpace_FILE(*UserSpace__imp___iob)[3];

static int UserSpace_getc(UserSpace_FILE *_Stream) {
	return getc(UserSpace_FileStructureReplace(_Stream));
}
static int UserSpace_putc(int _Character, UserSpace_FILE *_Stream) {
	return putc(_Character, UserSpace_FileStructureReplace(_Stream));
}
static int UserSpace_ungetc(int _Character, UserSpace_FILE *_Stream) {
	return ungetc(_Character, UserSpace_FileStructureReplace(_Stream));
}
static int UserSpace_fgetc(UserSpace_FILE *_Stream) {
	return fgetc(UserSpace_FileStructureReplace(_Stream));
}
static int UserSpace_fputc(int _Character, UserSpace_FILE *_Stream) {
	return fputc(_Character, UserSpace_FileStructureReplace(_Stream));
}
static unsigned int UserSpace_sleep(unsigned int seconds) {
	Sleep(1000 * seconds);
	return 0;
}

#define TccLibFuncInject_ReplaceSameNameSymbol(pState, name) (tcc_add_symbol(pState, #name, name))
#define TccLibFuncInject_ReplaceUserSpaceSymbol(pState, name) (tcc_add_symbol(pState, #name, UserSpace_ ## name))
bool static inline TccLibFuncInject_Early(TCCState *pState) {
	if (pState == NULL)
		return false;

	TccLibFuncInject_ReplaceSameNameSymbol(pState, printf);
	TccLibFuncInject_ReplaceSameNameSymbol(pState, scanf);
	TccLibFuncInject_ReplaceSameNameSymbol(pState, printf_s);
	TccLibFuncInject_ReplaceSameNameSymbol(pState, scanf_s);
	TccLibFuncInject_ReplaceSameNameSymbol(pState, gets_s);
	TccLibFuncInject_ReplaceSameNameSymbol(pState, puts);
	TccLibFuncInject_ReplaceSameNameSymbol(pState, getchar);
	TccLibFuncInject_ReplaceSameNameSymbol(pState, putchar);

	TccLibFuncInject_ReplaceUserSpaceSymbol(pState, sleep);
	TccLibFuncInject_ReplaceUserSpaceSymbol(pState, getc);
	TccLibFuncInject_ReplaceUserSpaceSymbol(pState, putc);
	TccLibFuncInject_ReplaceUserSpaceSymbol(pState, ungetc);
	TccLibFuncInject_ReplaceUserSpaceSymbol(pState, fgetc);
	TccLibFuncInject_ReplaceUserSpaceSymbol(pState, fputc);

	return true;
}

bool static inline TccLibFuncInject_Late(TCCState *pState) {
	UserSpace_FILE(**UserSpace_FileTemp)[3];

	if (pState == NULL)
		return false;

	UserSpace_FileTemp = (UserSpace_FILE(**)[3])tcc_get_symbol(pState, "_imp___iob");
	UserSpace__imp___iob = UserSpace_FileTemp != NULL ? *UserSpace_FileTemp : NULL;

	return true;
}

int static inline GetDigitCount(unsigned n) {
	//Assuming unsigned is 32-bit wide
	if (n < 10)
		return 1;
	if (n < 100)
		return 2;
	if (n < 1000)
		return 3;
	if (n < 10000)
		return 4;
	if (n < 100000)
		return 5;
	if (n < 1000000)
		return 6;
	if (n < 10000000)
		return 7;
	if (n < 100000000)
		return 8;
	if (n < 1000000000)
		return 9;
	return 10;	//Maximum digit count which unsigned 32-bit could reach
}

COLORREF static inline ColorBlend(COLORREF clr1, COLORREF clr2, uint8_t nClr1Weight) {
	//Normal version; faster one may be implemented by compiler or others
	register uint8_t r, g, b, r1, g1, b1, r2, g2, b2;
	r1 = GetRValue(clr1);
	g1 = GetGValue(clr1);
	b1 = GetBValue(clr1);
	r2 = GetRValue(clr2);
	g2 = GetGValue(clr2);
	b2 = GetBValue(clr2);
	r = (uint8_t)((r1 * nClr1Weight + r2 * (255 - nClr1Weight)) / 255);
	g = (uint8_t)((g1 * nClr1Weight + g2 * (255 - nClr1Weight)) / 255);
	b = (uint8_t)((b1 * nClr1Weight + b2 * (255 - nClr1Weight)) / 255);
	return RGB(r, g, b);
}

void static inline InitStdConsole(void) {
	AllocConsole();
	freopen("CONIN$", "r+t", stdin);
	freopen("CONOUT$", "w+t", stdout);
	freopen("CONOUT$", "w+t", stderr);
}

void static inline UninitStdConsole(void) {
	fclose(stderr);
	fclose(stdout);
	fclose(stdin);
	FreeConsole();
}

int static inline OpenSelectFileDlg(HWND hwParent, wchar_t *str, size_t nStrLen, uint32_t nFlags) {
	OPENFILENAME ofn = { 0 };
	bool bRet;

	if ((nFlags & OSFD_TYPE_OPEN) && (nFlags & OSFD_TYPE_SAVE))
		return OSFD_RET_FAILED;

	str[0] = L'\0';
	ofn.lStructSize = sizeof(OPENFILENAMEA);
	ofn.lpstrFilter = L"All files\0*.*\0";
	ofn.lpstrFile = str;
	ofn.nMaxFile = nStrLen;
	ofn.Flags = OFN_NOCHANGEDIR;

	if (nFlags & OSFD_TYPE_OPEN)
		bRet = (bool)GetOpenFileName(&ofn);
	if (nFlags & OSFD_TYPE_SAVE)
		bRet = (bool)GetSaveFileName(&ofn);

	if (bRet) {
		return OSFD_RET_SUCCEEDED;
	}
	else {
		return CommDlgExtendedError() == 0 ? OSFD_RET_USERCANCELLED : OSFD_RET_FAILED;
	}
}