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
 * @param pAddr      Address that will be edited.
 * @param iOffset    Bytes count. If value is negative,
 *                   then move pointer back.
 *
 * @return           Offsetter pAddr argument.
 */
stock Pointer Transpose(Pointer pAddr, int iOffset)
{
	return ptr(int(pAddr) + iOffset);
}

 /**
 * SourcePawn doesn't support native pointer dereference, so
 * we just need to read integer at the address instead.
 *
 * @param pAddr	     Address that will be edited.
 * @param iOffset    Offset relatively to pAddr. If value is negative,
 *                   then move pointer back.
 *                   
 * @return           Dereferenced value of pointer. If pAddr is null,
 *                   then -1 is going to be returned.
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
 * Convert absolute address to relative. Such assembler
 * command like "call" or "jmp" use absolute addresses,
 * so you need to convert them with this function.
 *
 * @param pAddr    Pointer at relative address in memory.
 *
 * @return         Absolute address.
 */
stock Pointer Absolute(Pointer pAddr)
{
	return ptr(int(pAddr) + Dereference(pAddr) + 5);
}

 /**
 * Opposite of Absolute function. Use it if you want to
 * write relative address to memory.
 *
 * @param pMemAddr    Pointer at place in memory where function will be inserted.
 * @param pAddr       Pointer at function in memory.
 *
 * @return            Address to pAddr relatively to pFunc.
 */
stock Pointer Relative(Pointer pMemAddr, Pointer pFunc)
{
	return ptr(int(pMemAddr) - int(pFunc) - 5);
}

/** Read Memory **/

 /**
 * Read byte from address.
 *
 * @param pAddr    Address where data will be readed.
 *                 
 * @return         Byte value.
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
 * Read word from address.
 *
 * @param pAddr    Address where data will be readed.
 *
 * @return         Word value.
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
 * Read integer from address.
 *
 * @param pAddr    Address where data will be readed.
 *
 * @return         Integer value.
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
 * Read fixed size array from address.
 *
 * @param pAddr     Address where data will be readed.
 * @param output    Destination address, value will be stored here.
 * @param iSize     Count of bytes for reading.
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
 * Write byte to address.
 *
 * @param pAddr    Address where value will be written.
 * @param value    Data for write.
 *
 * @return         Sum of pAddr and sizeof(byte).
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
 * Write word to address.
 *
 * @param pAddr    Address where value will be written.
 * @param value    Data for write.
 *
 * @return         Sum of pAddr and sizeof(word).
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
 * Write integer to address.
 *
 * @param pAddr    Address where value will be written.
 * @param value    Data for write.
 *
 * @return         Sum of pAddr and sizeof(int).
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
 * Write fixed size array to address.
 *
 * @param pAddr    Address where value will be written.
 * @param data     Array of bytes for write.
 * @param iSize    Length of data.
 *
 * @return         Sum of pAddr and the length of data.
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

 /**
 * Copies bytes from source address to destination.
 *
 * @param pAddr      Address where value will be written.
 * @param pMemory    Address of bytes for write.
 * @param iSize      Length of data.
 *
 * @return           Sum of pAddr and the length of data.
 */
stock Pointer WriteMemory(Pointer pAddr, Pointer pMemory, int iSize)
{
	if(pAddr == nullptr || pMemory == nullptr)
	{
		return nullptr;
	}
	
	for(int i = 0; i < iSize; i++)
	{
		int b = ReadByte(pMemory);
		WriteByte(pAddr, b);
		
		pAddr++;
		pMemory++;
	}
	
	return pAddr;
}

/** Search Memory **/

 /**
 * Returns the address of the first encountered byte.
 *
 * @param pStart     Starting point of search.
 * @param iValue     Searching data.
 * @param iOffset    Count of bytes that will be added to result.
 * @param bBack      Search direction.
 *
 * @return            Address to first detected byte.
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
 * Returns the address of the first encountered word.
 *
 * @param pStart     Starting point of search.
 * @param iValue     Searching data.
 * @param iOffset    Count of bytes that will be added to result.
 * @param bBack      Search direction.
 *
* @return            Address to first detected word.
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
 * Returns the address of the first encountered integer.
 *
 * @param pStart     Starting point of search.
 * @param iValue     Searching data.
 * @param iOffset    Count of bytes that will be added to result.
 * @param bBack      Search direction.
 *
 * @return           Address to first detected integer.
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
 * Returns the address of the first encountered byte pattern.
 * Use 0xFF byte value for skip this byte check.
 *
 * @param pStart          Starting point of search.
 * @param iSize           Count of bytes that needs to be checked.
 * @param pattern         Searching data.
 * @param iPatternSize    Length of pattern variable.
 * @param iOffset         Count of bytes that will be added to result.
 *                        
 * @return                Address to first detected pattern.
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

/* Check Memory */

 /**
 * Extracts byte value from address and compares it with argument.
 *
 * @param pAddr     Value address.
 * @param value     Value that will be compared with extracted one.
 * @param offset    Offset relative to pAddr.
 *                  
 * @return          True, if extracted value coincided with value in function 
 *                  argument.
 */
stock bool CheckByte(Pointer pAddr, char value, int offset)
{
	if (pAddr == nullptr)
	{
		return false;
	}
	
	return (Dereference(pAddr, offset) & 0xFF) == value;
}

 /**
 * Extracts word value from address and compares it with argument.
 *
 * @param pAddr     Value address.
 * @param value     Value that will be compared with extracted one.
 * @param offset    Offset relative to pAddr.
 *                  
 * @return          True, if extracted value coincided with value in function 
 *                  argument.
 */
stock bool CheckWord(Pointer pAddr, int value, int offset)
{
	if (pAddr == nullptr)
	{
		return false;
	}
	
	return (Dereference(pAddr, offset) & 0xFFFF) == value;
}

 /**
 * Extracts integer value from address and compares it with argument.
 *
 * @param pAddr     Value address.
 * @param value     Value that will be compared with extracted one.
 * @param offset    Offset relative to pAddr.
 *                  
 * @return          True, if extracted value coincided with value in function 
 *                  argument.
 */
stock bool CheckInt(Pointer pAddr, int value, int offset)
{
	if (pAddr == nullptr)
	{
		return false;
	}
	
	return (Dereference(pAddr, offset) & 0xFFFF) == value;
}

 /**
 * Checks address bound between start and end addresses.
 *
 * @param pStart    Start address.
 * @param pEnd      End address.
 * @param pAddr     Verifiable address.
 *                  
 * @return          True, if pAddr is not out of bound ranges.
 */
stock bool Bounds(Pointer pStart, Pointer pEnd, Pointer pAddr)
{
	return !(int(pAddr) < int(pStart) || int(pAddr) > int(pEnd))
}

/** Other **/

 /**
 * Find static memory region for dynamic code injection.
 *
 * @return    Safe place for code injection in server module.
 */
stock Pointer FindPlaceForCode()
{
	Handle h = LoadGameConfigFile("any.koh.memory");
	
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
 * Execute x86 assembler code.
 *
 * @param code    x86 assembler byte array.
 * @param size    Code size.
 *
 * @return        Result of assembler code execution.
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