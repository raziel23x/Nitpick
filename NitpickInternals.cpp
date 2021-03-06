#pragma warning(disable : 4005)
#include "NitpickInternals.h"

IDebugLog						gLog("Nitpick.log");
PluginHandle					g_pluginHandle = kPluginHandle_Invalid;

_DefineHookHdlr(INICollectionLoadSetting, 0x00AFEE53);
_DefineHookHdlr(TESDataHandlerPopulatePluginList, 0x0043E457);

void StartPickingNit()
{
	_MemHdlr(INICollectionLoadSetting).WriteJump();
	_MemHdlr(TESDataHandlerPopulatePluginList).WriteJump();
}

static char			s_GetPrivateProfileStringAuxBuffer[0x8000] = {0};		// large enough, I should think

#define _hhName	INICollectionLoadSetting
_hhBegin()
{
	_hhSetVar(Retn, 0x00AFEE74);
	__asm
	{
		push    0x8000
		lea		eax, s_GetPrivateProfileStringAuxBuffer
		push	eax
		push	ecx
		lea     edx, [esp + 0x74]
		push    edx
		lea     eax, [esp + 0x38]
		push    eax
		call    ebp			// GetPrivateProfileStringA
		lea		ecx, s_GetPrivateProfileStringAuxBuffer
		jmp		[_hhGetVar(Retn)]
	}
}

bool __stdcall FixPluginListPopulation(WIN32_FIND_DATA* FileData)
{
	static std::list<std::string> kActivePluginList;
	if (kActivePluginList.size() == 0)
	{
		const char* kAppDataPath = (const char*)0x01B9C2E8;
		const char* kPluginListName = ".\\Plugins.txt";

		char Buffer[0x104] = {0};
		strcpy_s(Buffer, sizeof(Buffer), kAppDataPath);
		strcat_s(Buffer, sizeof(Buffer), kPluginListName);

		std::fstream PluginListStream(Buffer, std::ios::in);
		if (PluginListStream.fail() == false)
		{
			char Entry[0x200] = {0};
			while (PluginListStream.eof() == false)
			{
				ZeroMemory(Entry, sizeof(Entry));
				PluginListStream.getline(Entry, sizeof(Entry));

				if (strlen(Entry) > 2 && Entry[0] != '#')
				{
					kActivePluginList.push_back(Entry);
				}
			}
		}
	}

	if (!_stricmp(FileData->cFileName, "skyrim.esm") ||
		!_stricmp(FileData->cFileName, "update.esm"))
	{
		return true;
	}

	for (std::list<std::string>::const_iterator Itr = kActivePluginList.begin(); Itr != kActivePluginList.end(); Itr++)
	{
		if (!_stricmp(Itr->c_str(), FileData->cFileName))
			return true;
	}

	return false;
}

#define _hhName	TESDataHandlerPopulatePluginList
_hhBegin()
{
	_hhSetVar(Retn, 0x0043E45E);
	_hhSetVar(Jump, 0x0043E5C3);
	__asm
	{
		lea     eax, [esp + 0x128]

		push	ecx
		push	esi
		push	eax
		call	FixPluginListPopulation
		pop		esi
		pop		ecx

		test	al, al
		jz		SKIPPLUGIN

		cmp     [esp + 0x144], ebp
		jmp		[_hhGetVar(Retn)]
	SKIPPLUGIN:
		jmp		[_hhGetVar(Jump)]
	}
}