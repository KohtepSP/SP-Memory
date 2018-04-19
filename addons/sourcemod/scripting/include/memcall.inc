/* 
 * TODO: Fix awful crutches like "sub ebp, 8" (but later);
 * TODO: Documentation and cleaning up unnecessary code comments.
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

#define FUNCCALL_MAX_ARGS 32

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

/** Function configuration **/
/* Calling convention */
static EFuncCallType g_FuncCallType = FuncCall_NoArgs;

static bool g_bIsWritingFunc = false;

static EFuncParamType g_FuncCallParams[FUNCCALL_MAX_ARGS];

static int g_FuncCallParamCount = 0;
/** **/

stock void PrepFunc_WriteCode(int[] code, int size)
{
	g_FuncCallOffsettedAddr = WriteData(g_FuncCallOffsettedAddr, code, size);
}

stock void PrepFunc_WriteByte(int value)
{
	g_FuncCallOffsettedAddr = WriteByte(g_FuncCallOffsettedAddr, value);
}

stock void PrepFunc_WriteWord(int value)
{
	g_FuncCallOffsettedAddr = WriteWord(g_FuncCallOffsettedAddr, value);
}

stock void PrepFunc_WriteInt(int value)
{
	g_FuncCallOffsettedAddr = WriteInt(g_FuncCallOffsettedAddr, value);
}

stock void StartPrepFuncCall(EFuncCallType callType, Pointer pFuncAddr)
{
	if(g_bIsWritingFunc)
	{
		return;
	}
	
	g_FuncOrigAddr = pFuncAddr;
	g_FuncCallAddr = FindPlaceForCode();
	g_FuncCallOffsettedAddr = g_FuncCallAddr;
	g_FuncCallType = callType;

	PrepFunc_WriteByte(0xCC);
	
	if (g_FuncCallType != FuncCall_NoArgs)
	{
		/*
			push ebp;
			mov ebp, dword ptr [esp + 16];
			sub ebp, 8; // temporaly, don't worry!
		*/
		PrepFunc_WriteCode({ 0x55, 0x8B, 0x6C, 0xE4, 0x10, 0x83, 0xED, 0x08 }, 8);
	}
	
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

	int i = int(Relative(g_FuncOrigAddr, g_FuncCallOffsettedAddr));
	
	PrepFunc_WriteByte(0xE8);
	PrepFunc_WriteInt(i);
	
	if(g_FuncCallParamCount > 0)
	{
		if(g_FuncCallType == FuncCall_CDecl)
		{
			/* add esp, g_FuncCallParamCount * 4; */
			PrepFunc_WriteCode({ 0x83, 0xC4 }, 2);
			PrepFunc_WriteByte(g_FuncCallParamCount * 4);
		}
	}

	if (g_FuncCallType != FuncCall_NoArgs)
	{
		/* pop ebp; */
		PrepFunc_WriteCode({ 0x5D }, 1);
	}
	
	/* ret; */
	PrepFunc_WriteCode({ 0xC3 }, 1);
	
	/* 
	    "funcInfo" is actually a struct, allocated in native memory.

		-8: Argument count; (thx sp for inability to use sizeof for indeterminate array size)
	    -4: Size of constructed function;
	    +0: Code itself.
	*/
	
	Pointer funcInfo;
	
	funcInfo = g_FuncCallAddr;
	WriteInt(Transpose(funcInfo, -4), int(g_FuncCallOffsettedAddr) - int(g_FuncCallAddr));
	WriteInt(Transpose(funcInfo, -8), g_FuncCallParamCount);

	/* Flush constructed function information. */
	g_FuncCallAddr = nullptr;
	g_FuncOrigAddr = nullptr;
	g_FuncCallParamCount = 0;
	g_bIsWritingFunc = false;
	g_FuncCallType = FuncCall_NoArgs;
	
	return funcInfo;
}

static Handle g_hFuncCall = null;
static Pointer g_pFuncCall = nullptr;

stock int FuncCall(Pointer pFuncInfo, int[] args)
{
	if (pFuncInfo == nullptr)	
	{
		return -1;
	}
	
	int nArgCount = ReadInt(Transpose(pFuncInfo, -8));
	int nFuncSize = ReadInt(Transpose(pFuncInfo, -4));
	Pointer pArgStruct = nullptr;
	Pointer pArgStructAddr = nullptr;

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
		
		/* Pointer to arguments structure. */
		PrepSDKCall_AddParameter(SDKType_PlainOldData, SDKPass_Plain);
		
		g_hFuncCall = EndPrepSDKCall();

		if(g_hFuncCall == null)
		{
			return -1;
		}

		pArgStruct = FindPlaceForCode();
		
		for (int j = 0; j <= nArgCount; j++)
		{
			PrintToServer("==== ---- %d", args[j]);
			pArgStruct = WriteInt(pArgStruct, args[j]);
		}
		
		pArgStruct = ptr(int(pArgStruct) - (nArgCount * 4));
		
		pArgStructAddr = FindPlaceForCode();
		WriteInt(pArgStructAddr, int(pArgStruct));
		
		/* Flush all "deadbeef" codes */		
		pAddr = g_pFuncCall;
		for(int i = 0; i < 4; i++)
		{
			pAddr = WriteInt(pAddr, 0x00000000);
		}
	}
		
	Pointer pAddr = g_pFuncCall;
	pAddr = WriteByte(pAddr, 0xCC);
	pAddr = WriteByte(pAddr, 0xE8);
	pAddr = WriteInt(pAddr, Relative(pFuncInfo, g_pFuncCall));
	WriteByte(pAddr, 0xC3);
	
	/* Call code at g_pFuncCall address */
	return SDKCall(g_hFuncCall, pArgStruct);
}

stock int FuncCall0(Pointer pFuncInfo)
{
	return FuncCall(pFuncInfo, { 0 });
}

stock int FuncCall1(Pointer pFuncInfo, int arg1)
{
	int args[1];
	args[0] = arg1;
	
	return FuncCall(pFuncInfo, args);
}

stock int FuncCall2(Pointer pFuncInfo, int arg1, int arg2)
{
	int args[2];
	args[0] = arg1;
	args[1] = arg2;
	
	return FuncCall(pFuncInfo, args);
}

stock int FuncCall3(Pointer pFuncInfo, int arg1, int arg2, int arg3)
{
	int args[3];
	args[0] = arg1;
	args[1] = arg2;
	args[2] = arg3;
	
	return FuncCall(pFuncInfo, args);
}

stock int FuncCall4(Pointer pFuncInfo, int arg1, int arg2, int arg3, int arg4)
{
	int args[4];
	args[0] = arg1;
	args[1] = arg2;
	args[2] = arg3;
	args[3] = arg4;
	
	return FuncCall(pFuncInfo, args);
}