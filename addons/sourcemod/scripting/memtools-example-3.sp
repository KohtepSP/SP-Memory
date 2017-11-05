#pragma semicolon 1
#pragma newdecls required

#include <memory\windows.inc>
#include <memory\memcall.inc>

public Plugin myinfo =
{
    name = "[ANY] MemTools Example #3",
    author = "2010kohtep",
    description = "An example plugin that calls WinAPI functions via FuncCall constructor.",
    version = "(DOES NOT WORK)",
    url = "https://github.com/2010kohtep/SP-Memory"
};

#define HEAP_ZERO_MEMORY 0x00000008

public void OnPluginStart()
{	
	Pointer pKernel32 = GetModuleHandle("kernel32.dll");
	Pointer pNtDll = GetModuleHandle("ntdll.dll");
	
	Pointer pGetProcessHeap = GetProcAddress(pKernel32, "GetProcessHeap");
	Pointer pRtlAllocateHeap = GetProcAddress(pNtDll, "RtlAllocateHeap");
	
	StartPrepFuncCall(FuncCall_NoArgs, pGetProcessHeap);
	Pointer pGetProcessHeapWrap = EndPrepFuncCall();

	StartPrepFuncCall(FuncCall_StdCall, pRtlAllocateHeap);
	PrepFuncCall_AddParameter(FuncParam_Int);
	PrepFuncCall_AddParameter(FuncParam_Int);
	PrepFuncCall_AddParameter(FuncParam_Int);
	Pointer pRtlAllocateHeapWrap = EndPrepFuncCall();
	
	PrintToServer("----------------------------------------");
	
	int hHeap = FuncCall0(pGetProcessHeapWrap);
	Pointer pMem = ptr(FuncCall3(pRtlAllocateHeapWrap, hHeap, HEAP_ZERO_MEMORY, 100));
	
	PrintToServer("[MemTools] Current process heap: 0x%08X", hHeap);
	PrintToServer("[MemTools] Native memory allocated at address 0x%08X", pMem);
	PrintToServer("----------------------------------------");
}