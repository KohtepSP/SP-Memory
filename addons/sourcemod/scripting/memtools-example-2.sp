#pragma semicolon 1
#pragma newdecls required

#include <memory\windows.inc>
#include <memory\memcall.inc>

public Plugin myinfo =
{
    name = "[ANY] MemTools Example #2",
    author = "2010kohtep",
    description = "An example plugin that calls Sleep() WinAPI function via StdCall().",
    version = "",
    url = "https://github.com/2010kohtep/SP-Memory"
};

public void OnPluginStart()
{	
	/* Get image base of kernel32. */
	Pointer pKernel32 = GetModuleHandle("kernel32.dll");
	/* Get user32.Sleep function pointer. */
	Pointer pSleep = GetProcAddress(pKernel32, "Sleep");
	
	PrintToServer("----------------------------------------");
	for(int i = 1; i <= 4; i++)
	{
		PrintToServer("[MemTools] Calling Sleep() with 1000 ms... (%d)", i);
		
		/* NOTE: StdCall function can only call functions with __stdcall convention 
		   and with 1 argument. */
		StdCall(pSleep, 1000);
		
		PrintToServer("[MemTools] Call done.");
	}
	PrintToServer("----------------------------------------");
}