#pragma semicolon 1
#pragma newdecls required

#include <memory\windows.inc>

public Plugin myinfo =
{
    name = "[ANY] MemTools Example #1",
    author = "2010kohtep",
    description = "An example plugin that retrieves \"kernel32.dll\" image base address and \"ExitProcess\" function pointer.",
    version = "",
    url = "https://github.com/2010kohtep/SP-Memory"
};

public void OnPluginStart()
{	
	/* Get image base of kernel32. */
	Pointer pKernel32 = GetModuleHandle("kernel32.dll");
	/* Get kernel32.ExitProcess function pointer. */
	Pointer pExitProcess = GetProcAddress(pKernel32, "ExitProcess");
	
	PrintToServer("----------------------------------------");
	PrintToServer("[MemTools] kernel32 = 0x%08X", pKernel32);
	PrintToServer("[MemTools] ExitProcess = 0x%08X", pExitProcess);
	PrintToServer("----------------------------------------");
}