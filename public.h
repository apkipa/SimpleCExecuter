#pragma once

#define TEXT_LANGUAGE_ID_CHINESE 1
#define TEXT_LANGUAGE_ID_ENGLISH 2

typedef int(*PtrFuncType_main)(int argc, char *argv[]);

typedef struct tagSeparateMode_SharedData {
	size_t nStructureSize;
	HWND hwnd;
	bool bUseConsole;
	bool bSeparated;
	bool bConsoleAutoPause;

	bool bReceived;

	char code[];
} SeparateMode_SharedData, *pSeparateMode_SharedData;

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