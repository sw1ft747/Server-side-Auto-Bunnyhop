// Copypasta from SvenMod

#ifndef MEMORYUTILS_H
#define MEMORYUTILS_H

#ifdef _WIN32
#pragma once
#endif

#include <Windows.h>

#include "udis86/udis86.h"
#include "hashtable.h"

//-----------------------------------------------------------------------------
// Macro definitions
//-----------------------------------------------------------------------------

#define DEFINE_PATTERN(name, signature) struct tpattern_s<get_pattern_length(signature), (unsigned char)0x2A> __pattern_##name(signature); \
	struct pattern_s *name = reinterpret_cast<struct pattern_s *>(&(__pattern_##name))

#define DEFINE_PATTERN_IGNORE_BYTE(name, signature, ignorebyte) struct tpattern_s<get_pattern_length(signature), (unsigned char)ignorebyte> __pattern_##name(signature); \
	struct pattern_s *name = reinterpret_cast<struct pattern_s *>(&(__pattern_##name))

#define DEFINE_NULL_PATTERN(name) struct pattern_s *name = 0

#define EXTERN_PATTERN(name) extern struct pattern_s *name

#define REPLACE_PATTERN(dest, src) dest = src

//-----------------------------------------------------------------------------
// Utils
//-----------------------------------------------------------------------------

static constexpr int hex_to_decimal_table[] =
{
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, 15, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

inline constexpr int hex_to_decimal_fast(char *hex)
{
	int result = 0;

	while (*hex && result >= 0)
	{
		result = (result << 4) | hex_to_decimal_table[*hex++];
	}

	return result;
}

static inline constexpr unsigned int get_pattern_length(const char *pszPattern)
{
	unsigned int nCount = 0;

	while (*pszPattern)
	{
		char symbol = *pszPattern;

		if (symbol != ' ')
		{
			++nCount;

			if (symbol != '?')
				++pszPattern;
		}

		++pszPattern;
	}

	return nCount;
}

template<unsigned int bytesCount, unsigned char ignoreByte>
struct tpattern_s
{
	constexpr tpattern_s(const char *pszPattern) : signature(), length(bytesCount), ignorebyte(ignoreByte)
	{
		unsigned int nLength = 0;

		while (*pszPattern)
		{
			char symbol = *pszPattern;

			if (symbol != ' ')
			{
				if (symbol != '?')
				{
					char byte[3] = { 0 };

					byte[0] = pszPattern[0];
					byte[1] = pszPattern[1];

					//signature[nLength] = static_cast<unsigned char>(strtoul(byte, NULL, 16));
					signature[nLength] = static_cast<unsigned char>(hex_to_decimal_fast(byte));

					++pszPattern;
				}
				else
				{
					signature[nLength] = ignorebyte;
				}

				++nLength;
			}

			++pszPattern;
		}
	}

	unsigned int length;
	unsigned char ignorebyte;
	unsigned char signature[bytesCount];
};

struct pattern_s
{
	unsigned int length;
	unsigned char ignorebyte;
	unsigned char signature;
};

struct moduleinfo_s
{
	void *pBaseOfDll;
	unsigned int SizeOfImage;
};

//-----------------------------------------------------------------------------
// Purpose: interface to memory's API
//-----------------------------------------------------------------------------

class CMemoryUtils
{
public:
	CMemoryUtils();
	~CMemoryUtils();

	//-----------------------------------------------------------------------------
	// Initialize ud structure
	//-----------------------------------------------------------------------------

	void InitDisasm(ud_t *instruction, void *buffer, uint8_t mode, size_t buffer_length = 128);

	//-----------------------------------------------------------------------------
	// Disassemble memory
	// For more abilities, use exported udis86 functions: public/udis86/include/extern.h
	//-----------------------------------------------------------------------------

	int Disassemble(ud_t *instruction);

	//-----------------------------------------------------------------------------
	// Set memory/page protection
	//-----------------------------------------------------------------------------

	bool VirtualProtect(void *pAddress, size_t size, int fNewProtect, int *pfOldProtect);

	//-----------------------------------------------------------------------------
	// Allocate virtual memory
	//-----------------------------------------------------------------------------

	void *VirtualAlloc(void *pAddress, size_t size, int fAllocationType, int fProtection);

	//-----------------------------------------------------------------------------
	// Free allocated virtual memory
	//-----------------------------------------------------------------------------

	bool VirtualFree(void *pAddress, size_t size, int fFreeType = 0);

	//-----------------------------------------------------------------------------
	// Patch memory address with given length
	//-----------------------------------------------------------------------------

	void PatchMemory(void *pAddress, unsigned char *pPatchBytes, int length);
	
	//-----------------------------------------------------------------------------
	// NOP memory address with given length
	//-----------------------------------------------------------------------------

	void MemoryNOP(void *pAddress, int length);

	//-----------------------------------------------------------------------------
	// Calculate absolute function address from CALL/JMP opcode
	// Pointer @pCallOpcode MUST point to CALL/JMP opcode
	//-----------------------------------------------------------------------------

	void *CalcAbsoluteAddress(void *pCallOpcode);

	//-----------------------------------------------------------------------------
	// Calculate relative address for calling/jumping from CALL/JMP opcode
	// Pointer @pFrom MUST point to CALL/JMP opcode
	//-----------------------------------------------------------------------------

	void *CalcRelativeAddress(void *pFrom, void *pTo);
	
	//-----------------------------------------------------------------------------
	// Get a virtual methods table
	//-----------------------------------------------------------------------------

	void *GetVTable(void *pClassInstance);
	
	//-----------------------------------------------------------------------------
	// Get a virtual function from virtual methods table
	//-----------------------------------------------------------------------------

	void *GetVirtualFunction(void *pClassInstance, int nFunctionIndex);

	//-----------------------------------------------------------------------------
	// Get module info
	//-----------------------------------------------------------------------------

	bool RetrieveModuleInfo(HMODULE hModule, moduleinfo_s *pModInfo);
	
	//-----------------------------------------------------------------------------
	// Lookup for a symbol in Symbol Table
	// In Windows, will be called GetProcAddress function
	//-----------------------------------------------------------------------------

	void *ResolveSymbol(HMODULE hModule, const char *pszSymbol);
	
	//-----------------------------------------------------------------------------
	// Find signature from pattern_s structure
	//-----------------------------------------------------------------------------

	void *FindPattern(HMODULE hModule, pattern_s *pPattern, unsigned int offset = 0);
	void *FindPatternWithin(HMODULE hModule, pattern_s *pPattern, void *pSearchStart, void *pSearchEnd);

	//-----------------------------------------------------------------------------
	// Find signature from string with given mask
	// If signature: "\xD9\x1D\x2A\x2A\x2A\x2A\x75\x0A\xA1", then mask: "xx????xxx"
	//-----------------------------------------------------------------------------

	void *FindPattern(HMODULE hModule, const char *pszPattern, char *pszMask, unsigned int offset = 0);
	void *FindPatternWithin(HMODULE hModule, const char *pszPattern, char *pszMask, void *pSearchStart, void *pSearchEnd);

	//-----------------------------------------------------------------------------
	// Find signature from string but ignore a specific byte
	// If signature: "\xD9\x1D\x2A\x2A\x2A\x2A\x75\x0A\xA1", then ignore byte can be: '0x2A'
	//-----------------------------------------------------------------------------

	void *FindPattern(HMODULE hModule, const char *pszPattern, unsigned int length, unsigned int offset = 0, char ignoreByte = '\x2A');
	void *FindPatternWithin(HMODULE hModule, const char *pszPattern, unsigned int length, void *pSearchStart, void *pSearchEnd, char ignoreByte = '\x2A');

	//-----------------------------------------------------------------------------
	// Find signature from range of bytes with a specific byte to ignore
	// For example: unsigned char sig[] = { 0xD9, 0x1D, 0x2A, 0x2A, 0x2A, 0x2A, 0x75, 0x0A, 0xA1 };
	//-----------------------------------------------------------------------------

	void *FindPattern(HMODULE hModule, unsigned char *pPattern, unsigned int length, unsigned int offset = 0, unsigned char ignoreByte = 0x2A);
	void *FindPatternWithin(HMODULE hModule, unsigned char *pPattern, unsigned int length, void *pSearchStart, void *pSearchEnd, unsigned char ignoreByte = 0x2A);

	//-----------------------------------------------------------------------------
	// Lookup for a string
	//-----------------------------------------------------------------------------

	void *FindString(HMODULE hModule, const char *pszString, unsigned int offset = 0);
	void *FindStringWithin(HMODULE hModule, const char *pszString, void *pSearchStart, void *pSearchEnd);

	//-----------------------------------------------------------------------------
	// Lookup for an address
	//-----------------------------------------------------------------------------

	void *FindAddress(HMODULE hModule, void *pAddress, unsigned int offset = 0);
	void *FindAddressWithin(HMODULE hModule, void *pAddress, void *pSearchStart, void *pSearchEnd);

private:
	moduleinfo_s m_ModuleInfo;
	CHashTable<HMODULE, moduleinfo_s> m_ModuleInfoTable;
};

extern CMemoryUtils g_MemoryUtils;
CMemoryUtils *MemoryUtils();

#endif // MEMORYUTILS_H