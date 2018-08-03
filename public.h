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

static unsigned int UserSpace_sleep(unsigned int seconds) {
	Sleep(1000 * seconds);
	return 0;
}

#define TccLibFuncInject_ReplaceSameNameSymbol(pState, name) (tcc_add_symbol(pState, #name, name))
#define TccLibFuncInject_ReplaceUserSpaceSymbol(pState, name) (tcc_add_symbol(pState, #name, UserSpace_ ## name))
bool static inline TccLibFuncInject_Early(TCCState *pState) {
	static FILE UserSpace__imp___iob[3];

	UserSpace__imp___iob[0] = *stdin;
	UserSpace__imp___iob[1] = *stdout;
	UserSpace__imp___iob[2] = *stderr;

	if (pState == NULL)
		return false;

	TccLibFuncInject_ReplaceSameNameSymbol(pState, printf);
	TccLibFuncInject_ReplaceSameNameSymbol(pState, scanf);
	TccLibFuncInject_ReplaceSameNameSymbol(pState, getchar);
	TccLibFuncInject_ReplaceSameNameSymbol(pState, putchar);

	tcc_add_symbol(pState, "sleep", UserSpace_sleep);
	tcc_add_symbol(pState, "getc", UserSpace_getc);
	tcc_add_symbol(pState, "putc", UserSpace_putc);

	return true;
}

bool static inline TccLibFuncInject_Late(TCCState *pState) {
	UserSpace_FILE(**UserSpace_FileTemp)[3];

	UserSpace_FileTemp = (UserSpace_FILE(**)[3])tcc_get_symbol(pState, "_imp___iob");
	if (UserSpace_FileTemp != NULL)
		UserSpace__imp___iob = *UserSpace_FileTemp;
	return true;
}