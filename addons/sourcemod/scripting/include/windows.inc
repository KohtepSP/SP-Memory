/**
 * Copyright (C) 2017, 2010kohtep
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
 
#if defined _WINDOWS_INCLUDED
	#endinput
#else
	#define _WINDOWS_INCLUDED
#endif
 
#include "memory.inc"

#define KUSER_SHARED_DATA 0x7FFE0000

static Pointer g_pPEB;

/**
 * Returns Microsoft Windows major and minor version numbers.
 *
 * @param iMajorVer     Output value for major version.
 * @param iMinorVer     Output value for major version
 *
 */
stock void GetWindowsVersion(int& iMajorVer, int& iMinorVer)
{
	Pointer pUserSharedData = ptr(KUSER_SHARED_DATA);
	
	iMajorVer = Dereference(pUserSharedData, 0x26C);
	iMinorVer = Dereference(pUserSharedData, 0x270);
}

/** 
 * Calculate virtual size of module, loaded in memory.
 * 
 * @param pAddr    Module image base.
 *
 * @return         Module size. 0 if argument is null, -1 if
 *                 address doesn't have a valid PE image signature.
 */
stock int GetModuleSize(Pointer pAddr)
{
	if(pAddr == nullptr)
	{
		return 0;
	}
	
	if(ReadWord(pAddr) == 0x5A4D) // MZ
	{
		int iOffset = Dereference(pAddr, 0x3C); // NT headers offset
		int iRes = Dereference(pAddr, iOffset + 0x50); // nt->OptionalHeader.SizeOfImage
		return iRes;
	}
	else
	{
		return -1;
	}
}

/** 
 * Retrieves a module handle for the specified module.
 *
 * @param name     The name of the loaded module.
 *
 * @return         Image base of the specified module name.
 */
stock Pointer GetModuleHandle(char[] name)
{
	if(GetCurrentPeb() == nullptr)
	{
		return nullptr;
	}
	
	/* If string is empty, then return EXE image base address */
	if(name[0] == '\0')
	{
		return ptr(Dereference(g_pPEB, 8)); // g_pPEB->ImageBaseAddress
	}
	
	Pointer pLdrModule = GetCurrentPeb();
	pLdrModule = ptr(Dereference(pLdrModule, 12)); // *(PEB_LDR_DATA*)g_pPEB->Ldr
	pLdrModule = ptr(Dereference(pLdrModule, 12)); // (LDR_MODULE*)Ldr->InMemoryOrderModuleList.Flink	
	
	/* DllBase of last list element is always nullptr */
	while(ptr(Dereference(pLdrModule, 24)) != nullptr) // pLdrModule->DllBase
	{
		char str[260];
		
		/* Get pointer at unicode buffer that is module name */
		Pointer pDllName = ptr(Dereference(pLdrModule, 48)); // pLdrModule->BaseDllName.Buffer
		
		int i = 0;
		while(pLdrModule)
		{
			/*  WideChar to AnsiChar convertor, We can do this freely because,
				all _default_ modules don't even contain unicode names */
			str[i] = ReadWord(pDllName);
			pDllName = Transpose(pDllName, 2);
			
			if(str[i] == '\0')
			{
				break;
			}
			
			i++;
		}
		
		/* If name is equal to str, then return current module's DllBase value */
		if(strcmp(str, name, false) == 0)
		{
			return ptr(Dereference(pLdrModule, 24));
		}
		
		/* Go to the next module */
		pLdrModule = ptr(Dereference(pLdrModule)); // (LDR_MODULE*)pModule->InLoadOrder.Flink;
	}
	
	return nullptr;
}

/** 
 * Retrieves the address of an exported function or 
 * variable from the specified dynamic-link library (DLL).
 *
 * @param pModule    A pointer to the DLL module that contains the function or variable.
 * @param name       The function or variable name.
 *
 * @return           Address of the exported function or variable.
 */
stock Pointer GetProcAddress(Pointer pModule, char[] name)
{
	Pointer pDOSHeader  = pModule;
	Pointer pPEHeader   = Transpose(pDOSHeader, Dereference(pDOSHeader, 60));
	Pointer pImportDesc = Transpose(pModule,  Dereference(pPEHeader, 120));
		
	int iNamesCount   = Dereference(pImportDesc, 24);
	Pointer pFuncList = Transpose(pModule, Dereference(pImportDesc, 28));
	Pointer pNameList = Transpose(pModule, Dereference(pImportDesc, 32));
	Pointer pOrdList  = Transpose(pModule, Dereference(pImportDesc, 36));
	
	for(int i = 0; i < iNamesCount; i++)
	{
		char strBuf[1024];
		
		Pointer pStrName = Transpose(pModule, Dereference(pNameList));
		
		/*  PCHAR to char[] */
		int j = 0;
		while(pModule)
		{
			if(j >= 1024)
			{
				break;
			}
			
			strBuf[j] = ReadByte(pStrName);
			
			if(strBuf[j] == '\0')
			{
				break;
			}
			
			j++;
			pStrName = Transpose(pStrName, 1);
		}
		strBuf[1023] = '\0';
		/* */
		
		if(strcmp(strBuf, name, false) == 0)
		{
			int iFuncNum = (Dereference(pOrdList) & 0xFFFF) * 4;
			Pointer pFuncAddr = ptr(int(pFuncList) + iFuncNum);
			
			return Transpose(pModule, Dereference(pFuncAddr));
		}
		
		pNameList = Transpose(pNameList, 4);
		pOrdList  = Transpose(pOrdList, 2);
	}
	
	return nullptr;
}

/** 
 * Returns a pointer to the Thread Environment Block (TEB) of the current thread.
 *
 * @return    A pointer to the thread environment block of the current thread.
 */
stock Pointer GetCurrentPeb()
{
	static int g_GetPEBCode[] = 
	{
		0x64, 0xA1, 0x30, 0x00, 0x00, 0x00, // mov eax, dword ptr fs:[30]
		0xC3                                // ret
	};	
	
	return ptr(AsmCall(g_GetPEBCode, 7));
}