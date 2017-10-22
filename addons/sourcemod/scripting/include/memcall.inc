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
 
/* 
*   ATTENTION: All functional below is not tested yet, it probably doesn't even work! 
*/
 
#if defined _MEMCALL_INCLUDED
	#endinput
#else
	#define _MEMCALL_INCLUDED
#endif
 
 #pragma semicolon 1
 
#include "memory.inc"

static Handle g_hStdCall = null;
static Pointer g_pStdCall = nullptr;

int g_StdCall[] = 
{
	0x58,				// pop eax (save return address)
	0x8B, 0x0C, 0x24,	// mov ecx, dword ptr [esp] (restore pFunc argument)
	0x89, 0x04, 0x24,	// mov dword ptr [esp], eax
	0xFF, 0xE1			// jmp ecx
};

stock int StdCall(Pointer pFunc, int iArg)
{
	if(GetCurrentPeb() == nullptr)
	{
		return -1;
	}
	
	Pointer pAddr = GetModuleHandle("server.dll");
	
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
	
	/* Save address for our code */
	g_pStdCall = pAddr;
	
	/* Add unique signature for PrepSDKCall_SetSignature() call */
	for(int i = 0; i < 4; i++)
	{
		StoreToAddress(pAddr, 0xDEADBEEF, NumberType_Int32);
		pAddr = Transpose(pAddr, 4);
	}
	
	/* SDKCall_Static means don't use "this" argument */
	StartPrepSDKCall(SDKCall_Static);
	
	/* Create SDK call with our asm memory pointer */
	PrepSDKCall_SetSignature(SDKLibrary_Server, "\xEF\xBE\xAD\xDE\xEF\xBE\xAD\xDE\xEF\xBE\xAD\xDE\xEF\xBE\xAD\xDE", 16);
	PrepSDKCall_AddParameter(SDKType_PlainOldData, SDKPass_Plain);
	PrepSDKCall_AddParameter(SDKType_PlainOldData, SDKPass_Plain);
	PrepSDKCall_SetReturnInfo(SDKType_PlainOldData, SDKPass_Plain);
	g_hStdCall = EndPrepSDKCall();

	if(g_hStdCall == null)
	{
		return -1;
	}
	
	/* Flush all "deadbeef" codes */		
	pAddr = g_pStdCall;
	for(int i = 0; i < 4; i++)
	{
		StoreToAddress(pAddr, 0x00000000, NumberType_Int32);
		pAddr = Transpose(pAddr, 4);
	}
	
	/* Write x86 assembler code */
	WriteData(g_pStdCall, g_StdCall, 10);
	
	/* Call code at g_pStdCall address */
	return SDKCall(g_hStdCall, pFunc, iArg);	
}

/*
	StartPrepSDKCall(SDKCall_Static);
	PrepSDKCall_SetSignature(SDKLibrary_Server, "\xEF\xBE\xAD\xDE\xEF\xBE\xAD\xDE\xEF\xBE\xAD\xDE\xEF\xBE\xAD\xDE", 16);
	PrepSDKCall_AddParameter(SDKType_PlainOldData, SDKPass_Plain);
	PrepSDKCall_AddParameter(SDKType_PlainOldData, SDKPass_Plain);
	PrepSDKCall_SetReturnInfo(SDKType_PlainOldData, SDKPass_Plain);
	g_hStdCall = EndPrepSDKCall();
	SDKCall(g_hStdCall, pFunc, iArg);

*/

#define FUNCCALL_MAX_ARGS 32

struct TFuncInfo
{
	public int iArgCount;
	public int iArgList[FUNCCALL_MAX_ARGS];
	public Pointer pFuncAdd;
};

/* TODO: __pascal, __register, __vectorcall */
enum EFuncCallType
{
	/* Callee cleans the stack, so we don't need to generate "retn NUM" instruction */
	FuncCall_StdCall,
	/* Caller cleans the stack, needs to generate "retn NUM" instruction */
	FuncCall_CDecl,
	/* First two arguments are registers (ecx, edx), callee cleans the stack */
	FuncCall_FastCall,
	/* No arguments used */
	FuncCall_NoArgs
};

enum EFuncParamType
{
	FuncParam_Int,
	FuncParam_Float,	/* Not implemented */
	FuncParam_Pointer	/* Not implemented */
};

/* Original address of calling function */
static Pointer g_FuncOrigAddr = nullptr;
/* Begin of our code execution */
static Pointer g_FuncCallAddr = nullptr;
/* Current code writing position */
static Pointer g_FuncCallOffsettedAddr = nullptr;

static EFuncCallType g_FuncCallType = FuncCall_NoArgs;
static bool g_bIsWritingFunc = false;
static EFuncParamType g_FuncCallParams[FUNCCALL_MAX_ARGS];
static int g_FuncCallParamCount = 0;

static TFuncInfo g_FuncInfo;

stock void PrepFunc_WriteCode(int[] code, int size)
{
	g_FuncCallOffsettedAddr = WriteData(g_FuncCallOffsettedAddr, code, size);
}

stock void PrepFunc_WriteByte(int value)
{
	g_FuncCallOffsettedAddr = WriteByte(g_FuncCallOffsettedAddr, value);
}

stock void StartPrepFuncCall(EFuncCallType value, Pointer pFuncAddr)
{
	if(g_bIsWritingFunc != false)
	{
		return;
	}
	
	g_FuncOrigAddr = pFuncAddr;
	g_FuncCallAddr = FindPlaceForCode();
	g_FuncCallOffsettedAddr = g_FuncCallAddr;
	g_FuncCallType = value;
	
	/*
		push ebp;
		mov ebp, esp;
	*/
	PrepFunc_WriteCode({ 0x55, 0x8B, 0xEC }, 3);
	
	g_bIsWritingFunc = true;
}

stock void PrepFuncCall_AddParameter(EFuncParamType value)
{
	if(g_FuncCallParamCount == FUNCCALL_MAX_ARGS)
	{
		return;
	}
	
	g_FuncCallParams[g_FuncCallParamCount] = value;
	g_FuncCallParamCount++;
}

/*

ebp+20 = ...
ebp+16 = arg3
ebp+12 = arg2
ebp+8  = arg1
ebp+4  = ret addr
ebp+0  = esp value

*/
stock Pointer EndPrepFuncCall()
{
	if(g_bIsWritingFunc != true)
	{
		return nullptr;
	}
	
	if(g_FuncCallType == FuncCall_FastCall)
	{
		if(g_FuncCallParamCount == 1)
		{
			/* mov ecx, dword ptr [ebp + 8]; */
			PrepFunc_WriteCode({ 0x8B, 0x4D, 0x08 }, 3);			
		}
		else if(g_FuncCallParamCount == 2)
		{
			/* mov edx, dword ptr [ebp + 12]; */
			PrepFunc_WriteCode({ 0x8B, 0x55, 0x0C }, 3);
			
			PrepFunc_WriteCode({ 0x8B, 0x4D, 0x08 }, 3);		
		}
		else
		{
			for(int i = 0; i < g_FuncCallParamCount - 2; i++)
			{
				/* push dword ptr [ebp + 16 + i * 4]; */
				PrepFunc_WriteCode({ 0xFF, 0x75 }, 2);
				PrepFunc_WriteByte(0x10 + i * 4);
			}
			
			PrepFunc_WriteCode({ 0x8B, 0x55, 0x0C }, 3);
			PrepFunc_WriteCode({ 0x8B, 0x4D, 0x08 }, 3);			
		}
	}
	else
	{
		for(int i = 0; i < g_FuncCallParamCount; i++)
		{
			/* push dword ptr [ebp + 8 + i * 4]; */
			PrepFunc_WriteCode({ 0xFF, 0x75 }, 2);
			PrepFunc_WriteByte(0x08 + i * 4);
		}
	}
	
	PrepFunc_WriteByte(0xE8);
	WriteInt(g_FuncCallOffsettedAddr, int(Relative(g_FuncCallOffsettedAddr, g_FuncOrigAddr)));
	
	if(g_FuncCallType == FuncCall_CDecl && g_FuncCallParamCount > 0)
	{
		/* add esp, g_FuncCallParamCount * 4; */
		PrepFunc_WriteCode({ 0x83, 0xC4 }, 2);
		PrepFunc_WriteByte(g_FuncCallParamCount * 4);
	}
	
	/* ret; */
	PrepFunc_WriteCode({ 0xC3 }, 1);		
	
	Pointer pRes = g_FuncCallAddr;
	
	g_FuncCallAddr = nullptr;
	g_FuncOrigAddr = nullptr;
	g_FuncCallParamCount = 0;
	g_bIsWritingFunc = false;
	g_FuncCallType = FuncCall_NoArgs;
	
	
	return pRes;
}

static Handle g_hFuncCall = null;
static Pointer g_pFuncCall = nullptr;

stock int FuncCall(Pointer pFuncAddr)
{
	if(g_hFuncCall == null)
	{
		/* Get free space for our code */
		g_pFuncCall = FindPlaceForCode();
		
		Pointer pAddr = g_pFuncCall;
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
		g_hFuncCall = EndPrepSDKCall();

		if(g_hFuncCall == null)
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
	
	/* Call code at g_pFuncCall address */
	return SDKCall(g_hFuncCall);	
}