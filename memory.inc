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

#if defined _MEMORY_INCLUDED
	#endinput
#else
	#define _MEMORY_INCLUDED
#endif

#pragma semicolon 1

#include <sourcemod>
#include <sdktools>

/* Pointer type means pointer at "native" process memory */
#define Pointer Address
#define nullptr Address_Null

/* Pretty comfortable way for type conversion */
#define int(%1) view_as<int>(%1)
#define ptr(%1) view_as<Pointer>(%1)

/** Common **/

/**
 * Edits pointer value without type casting.
 *
 * @param pAddr			Address that will be edited.
 * @param iOffset		Bytes count. If value is negative,
 *						then move pointer back.
 *
 * @return				Offsetter pAddr argument.
 */
stock Pointer Transpose(Pointer pAddr, int iOffset)
{
	return ptr(int(pAddr) + iOffset);
}

 /**
 *	SourcePawn doesn't support native pointer dereference, so
 *	we just need to read integer at the address instead.
 *
 * @param pAddr			Address that will be edited.
 * @param iOffset		Offset relatively to pAddr. If value is negative,
 *						then move pointer back.
 *
 * @return				Dereferenced value of pointer. If pAddr is null,
 *						then -1 is going to be returned.
 */
stock int Dereference(Pointer pAddr, int iOffset = 0)
{
	if(pAddr == nullptr)
	{
		return -1;
	}
	
	return ReadInt(Transpose(pAddr, iOffset));
}

 /**
 *	Convert absolute address to relative. Such assembler
 *  command like "call" or "jmp" use absolute addresses,
 *  so you need to convert them with this function.
 *
 * @param pAddr			Pointer at relative address in memory.
 *
 * @return				Absolute address relatively to value at pAddr.
 */
stock Pointer Absolute(Pointer pAddr)
{
	int iRes = 0;
	iRes += int(pAddr);
	iRes += Dereference(pAddr);
	iRes += sizeof(pAddr);
		
	return ptr(iRes);
}

 /**
 *	Opposite of Absolute function. Use it if you want to
 *  write relative address to memory.
 *
 * @param pAddr
 * @param pFunc
 *
 * @return
 */
stock Pointer Relative(Pointer pFunc, Pointer pAddr)
{
	// int iRes;
	// iRes = int(pFunc);
	// iRes -= int(pAddr);
	// iRes -= 4; // sizeof(pAddr)
	
	return int(pAddr) - int(pFunc) - 5;
	
	//return ptr(iRes);
}

/** Read Memory **/

 /**
 *	Read byte from address.
 *
 * @param pAddr
 *
 * @return
 */
stock int ReadByte(Pointer pAddr)
{
	if(pAddr == nullptr)
	{
		return -1;
	}
	
	return LoadFromAddress(pAddr, NumberType_Int8);
}

 /**
 *	Read word from address.
 *
 * @param pAddr
 *
 * @return
 */
stock int ReadWord(Pointer pAddr)
{
	if(pAddr == nullptr)
	{
		return -1;
	}
	
	return LoadFromAddress(pAddr, NumberType_Int16);
}

 /**
 *	Read integer from address.
 *
 * @param pAddr
 *
 * @return
 */
stock int ReadInt(Pointer pAddr)
{
	if(pAddr == nullptr)
	{
		return -1;
	}
	
	return LoadFromAddress(pAddr, NumberType_Int32);
}

 /**
 *	Read fixed size array from address.
 *
 * @param pAddr
 * @param output
 * @param iSize
 *
 */
stock void ReadData(Pointer pAddr, int[] output, int iSize)
{
	if(pAddr == nullptr)
	{
		return;
	}
	
	for(int i = 0; i < iSize; i++)
	{
		output[i] = ReadInt(pAddr);
		pAddr++;
	}
}

/** Write Memory **/

 /**
 *	Write byte to address.
 *
 * @param pAddr
 * @param value
 *
 * @return
 */
stock Pointer WriteByte(Pointer pAddr, char value)
{
	if(pAddr == nullptr)
	{
		return nullptr;
	}
	
	StoreToAddress(ptr(pAddr), int(value), NumberType_Int8);
	return ptr(int(pAddr) + 1);
}

 /**
 *	Write word to address.
 *
 * @param pAddr
 * @param value
 *
 * @return
 */
stock Pointer WriteWord(Pointer pAddr, int value)
{
	if(pAddr == nullptr)
	{
		return nullptr;
	}
	
	StoreToAddress(ptr(pAddr), value, NumberType_Int16);
	return ptr(int(pAddr) + 2);
}

 /**
 *	Write integer to address.
 *
 * @param pAddr
 * @param value
 *
 * @return
 */
stock Pointer WriteInt(Pointer pAddr, int value)
{
	if(pAddr == nullptr)
	{
		return nullptr;
	}
	
	StoreToAddress(ptr(pAddr), value, NumberType_Int32);
	return ptr(int(pAddr) + 4);
}

 /**
 *	Write fixed size array to address.
 *
 * @param pAddr
 * @param data
 * @param iSize
 *
 * @return
 */
stock Pointer WriteData(Pointer pAddr, int[] data, int iSize)
{
	if(pAddr == nullptr)
	{
		return nullptr;
	}
	
	for(int i = 0; i < iSize; i++)
	{
		WriteByte(pAddr, data[i]);
		
		pAddr++;
	}
	
	return pAddr;
}

/** Search Memory **/

 /**
 *	Returns the address of the first encountered byte.
 *
 * @param pStart
 * @param iValue
 * @param iOffset
 * @param bBack
 *
 * @return
 */
stock Pointer FindBytePtr(Pointer pStart, int iValue, int iOffset, bool bBack)
{
	if(pStart == nullptr)
	{
		return nullptr;
	}
	
	while(pStart)
	{
		if(ReadByte(pStart) == iValue)
		{
			return ptr(int(pStart) + iOffset);
		}
		
		pStart = Pointer(int(pStart) + 1);
	}
}

 /**
 *	Returns the address of the first encountered word.
 *
 * @param pStart
 * @param iValue
 * @param iOffset
 * @param bBack
 *
 * @return
 */
stock Pointer FindWordPtr(Pointer pStart, int iValue, int iOffset, bool bBack)
{
	if(pStart == nullptr)
	{
		return nullptr;
	}
	
	while(pStart)
	{
		if(ReadWord(pStart) == iValue)
		{
			return ptr(int(pStart) + iOffset);
		}
		
		pStart = Pointer(int(pStart) + 2);
	}
}

 /**
 *	Returns the address of the first encountered integer.
 *
 * @param pStart
 * @param iValue
 * @param iOffset
 * @param bBack
 *
 * @return
 */
stock Pointer FindIntPtr(Pointer pStart, int iValue, int iOffset, bool bBack)
{
	if(pStart == nullptr)
	{
		return nullptr;
	}
	
	while(pStart)
	{
		if(ReadInt(pStart) == iValue)
		{
			return ptr(int(pStart) + iOffset);
		}
		
		pStart = Pointer(int(pStart) + 4);
	}
}

 /**
 *	Returns the address of the first encountered byte pattern.
 *  Use 0xFF byte value for skip this byte check.
 *
 * @param pStart
 * @param iSize
 * @param pattern
 * @param iPatternSize
 * @param iOffset
 *
 * @return
 */
stock Pointer FindPattern(Pointer pStart, int iSize, int[] pattern, int iPatternSize, int iOffset)
{
	for(int i = 0; i < iSize; i++)
	{
		if(ReadByte(pStart) == pattern[0])
		{
			bool bFound = true;
			
			for(int j = 1; j < iPatternSize; j++)
			{
				int iDestByte = ReadByte(ptr(int(pStart) + j));
				int iSrcByte = pattern[j];

				if(iSrcByte != 0xFF)
				{
					if(iDestByte != iSrcByte)
					{
						bFound = false;
						break;
					}
				}
			}
			
			if(bFound)
			{
				return ptr(int(pStart) + iOffset); 
			}
		}
		
		pStart++;
	}

	return nullptr;
}

/** Other **/

 /**
 *	Find static memory region for dynamic code injection.
 *
 * @return				Safe place for code injection in server module.
 */
stock Pointer FindPlaceForCode()
{
	Handle h = LoadGameConfigFile("any.tutor.memory");
	
	if(h == null)
	{
		return nullptr;
	}
	
	Pointer pAddr = GameConfGetAddress(h, "server");
	
	delete h;  
	
	if(pAddr == nullptr)
	{
		return nullptr;
	}
	
	pAddr = ptr(int(pAddr) + GetModuleSize(pAddr) - 1);
	
	/* I could use "true", but compiler blames this line with "redundant test" */
	while(pAddr)
	{
		int b = ReadByte(pAddr);
		
		if(b != 0x00)
		{
			break;
		}
		
		pAddr--;
	}
		
	/* Align for safe code injection */
	pAddr = ptr(int(pAddr) + 0x20 & 0xFFFFFFF0);

	return pAddr;	
}

static Handle g_hAsmCall = null;
static Pointer g_pAsmCall = nullptr;

 /**
 *	Execute x86 assembler code.
 *
 * @param code			x86 assembler byte array.
 * @param size			Code size.
 *
 * @return				Result of assembler code execution.
 */
stock int AsmCall(int[] code, int size)
{
	if(g_hAsmCall == null)
	{
		/* Get free space for our code */
		g_pAsmCall = FindPlaceForCode();
		
		Pointer pAddr = g_pAsmCall;
		/* Add unique signature for PrepSDKCall_SetSignature() call */
		for(int i = 0; i < 4; i++)
		{
			pAddr = WriteInt(pAddr, 0xDEADBEEF);
		}
		
		/* SDKCall_Static means don't use "this" argument */
		StartPrepSDKCall(SDKCall_Static);
		
		/* Create SDK call with our asm memory pointer */
		PrepSDKCall_SetSignature(SDKLibrary_Server, "\xEF\xBE\xAD\xDE\xEF\xBE\xAD\xDE\xEF\xBE\xAD\xDE\xEF\xBE\xAD\xDE", 16);
		PrepSDKCall_SetReturnInfo(SDKType_PlainOldData, SDKPass_Plain);
		g_hAsmCall = EndPrepSDKCall();

		if(g_hAsmCall == null)
		{
			return -1;
		}
		
		/* Flush all "deadbeef" codes */		
		pAddr = g_pAsmCall;
		for(int i = 0; i < 4; i++)
		{
			pAddr = WriteInt(pAddr, 0x00000000);
		}
	}
	
	/* Write x86 assembler code */
	WriteData(g_pAsmCall, code, size);
	
	/* Call code at g_pAsmCall address */
	return SDKCall(g_hAsmCall);
}