#include "memory_utils.h"

//-----------------------------------------------------------------------------
// CMemoryUtils implementation
//-----------------------------------------------------------------------------

CMemoryUtils::CMemoryUtils() : m_ModuleInfo(), m_ModuleInfoTable(15)
{
}

CMemoryUtils::~CMemoryUtils()
{
	m_ModuleInfoTable.Purge();
}

//-----------------------------------------------------------------------------
// Disassembler
//-----------------------------------------------------------------------------

void CMemoryUtils::InitDisasm(ud_t *instruction, void *buffer, uint8_t mode, size_t buffer_length /* = 128 */)
{
	ud_init(instruction);
	ud_set_mode(instruction, mode);
	ud_set_input_buffer(instruction, (const uint8_t *)buffer, buffer_length);
}

int CMemoryUtils::Disassemble(ud_t *instruction)
{
	return ud_disassemble(instruction);
}

//-----------------------------------------------------------------------------
// Virtual memory
//-----------------------------------------------------------------------------

bool CMemoryUtils::VirtualProtect(void *pAddress, size_t size, int fNewProtect, int *pfOldProtect)
{
	return !!(::VirtualProtect(pAddress, size, fNewProtect, (PDWORD)pfOldProtect));
}

void *CMemoryUtils::VirtualAlloc(void *pAddress, size_t size, int fAllocationType, int fProtection)
{
	return ::VirtualAlloc(pAddress, size, fAllocationType, fProtection);
}
	
bool CMemoryUtils::VirtualFree(void *pAddress, size_t size, int fFreeType /* = 0 */)
{
#pragma warning(push)
#pragma warning(disable : 28160)

	return !!(::VirtualFree(pAddress, size, fFreeType));

#pragma warning(pop)
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CMemoryUtils::PatchMemory(void *pAddress, unsigned char *pPatchBytes, int length)
{
	int dwProtection;

	this->VirtualProtect(pAddress, length, PAGE_EXECUTE_READWRITE, &dwProtection);

	memcpy(pAddress, pPatchBytes, length);

	this->VirtualProtect(pAddress, length, dwProtection, NULL);
}

void CMemoryUtils::MemoryNOP(void *pAddress, int length)
{
	int dwProtection;

	this->VirtualProtect(pAddress, length, PAGE_EXECUTE_READWRITE, &dwProtection);

	memset(pAddress, 0x90, length);

	this->VirtualProtect(pAddress, length, dwProtection, NULL);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void *CMemoryUtils::CalcAbsoluteAddress(void *pCallAddress)
{
	unsigned long luRelativeAddress = *(unsigned long *)((unsigned char *)pCallAddress + 1);
	return (void *)(luRelativeAddress + (unsigned long)pCallAddress + sizeof(void *) + 1);
}

void *CMemoryUtils::CalcRelativeAddress(void *pFrom, void *pTo)
{
	return (void *)((unsigned long)pTo - ((unsigned long)pFrom + sizeof(void *) + 1));
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void *CMemoryUtils::GetVTable(void *pClassInstance)
{
	void *pVTable = *static_cast<void **>(pClassInstance);
	return pVTable;
}

void *CMemoryUtils::GetVirtualFunction(void *pClassInstance, int nFunctionIndex)
{
	void **pVTable = *static_cast<void ***>(pClassInstance);
	return reinterpret_cast<void *>(pVTable[nFunctionIndex]);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool CMemoryUtils::RetrieveModuleInfo(HMODULE hModule, moduleinfo_s *pModInfo)
{
	if ( !hModule )
		return false;

	moduleinfo_s *pHashEntry = NULL;

	if ( pHashEntry = m_ModuleInfoTable.Find(hModule) )
	{
		*pModInfo = *pHashEntry;
		return true;
	}

	MEMORY_BASIC_INFORMATION memInfo;

	if ( VirtualQuery(hModule, &memInfo, sizeof(MEMORY_BASIC_INFORMATION)) )
	{
		if ( memInfo.State != MEM_COMMIT || memInfo.Protect == PAGE_NOACCESS )
			return false;

		unsigned int dwAllocationBase = (unsigned int)memInfo.AllocationBase;
		pModInfo->pBaseOfDll = memInfo.AllocationBase;

		IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)dwAllocationBase;
		IMAGE_NT_HEADERS *pe = (IMAGE_NT_HEADERS *)(dwAllocationBase + dos->e_lfanew);

		IMAGE_FILE_HEADER *file = &pe->FileHeader;
		IMAGE_OPTIONAL_HEADER *opt = &pe->OptionalHeader;

		if (dos->e_magic == IMAGE_DOS_SIGNATURE && pe->Signature == IMAGE_NT_SIGNATURE && opt->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC && file->Machine == IMAGE_FILE_MACHINE_I386)
		{
			/*
			if ( (file->Characteristics & IMAGE_FILE_DLL) == 0 )
				return false;
			*/
		
			pModInfo->SizeOfImage = opt->SizeOfImage;

			m_ModuleInfoTable.Insert(hModule, *pModInfo);

			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void *CMemoryUtils::ResolveSymbol(HMODULE hModule, const char *pszSymbol)
{
	return GetProcAddress(hModule, pszSymbol);
}

//-----------------------------------------------------------------------------
// Sig scanner
//-----------------------------------------------------------------------------

void *CMemoryUtils::FindPattern(HMODULE hModule, pattern_s *pPattern, unsigned int offset /* = 0 */)
{
	if ( RetrieveModuleInfo(hModule, &m_ModuleInfo) )
	{
		unsigned long nLength = pPattern->length;
		unsigned char *pSignature = &pPattern->signature;

		unsigned char *pSearchStart = (unsigned char *)m_ModuleInfo.pBaseOfDll + offset;
		unsigned char *pSearchEnd = pSearchStart + m_ModuleInfo.SizeOfImage - nLength;

		while (pSearchStart < pSearchEnd)
		{
			bool bFound = true;

			for (register unsigned long i = 0; i < nLength; i++)
			{
				if (pSignature[i] != pPattern->ignorebyte && pSignature[i] != pSearchStart[i])
				{
					bFound = false;
					break;
				}
			}

			if (bFound)
				return (void *)pSearchStart;

			pSearchStart++;
		}
	}

	return NULL;
}

void *CMemoryUtils::FindPattern(HMODULE hModule, const char *pszPattern, char *pszMask, unsigned int offset /* = 0 */)
{
	if ( RetrieveModuleInfo(hModule, &m_ModuleInfo) )
	{
		unsigned long nMaskLength = strlen(pszMask);

		unsigned char *pSearchStart = (unsigned char *)m_ModuleInfo.pBaseOfDll + offset;
		unsigned char *pSearchEnd = pSearchStart + m_ModuleInfo.SizeOfImage - nMaskLength;

		while (pSearchStart < pSearchEnd)
		{
			bool bFound = true;

			for (register unsigned long i = 0; i < nMaskLength; i++)
			{
				if (pszMask[i] != '?' && pszPattern[i] != pSearchStart[i])
				{
					bFound = false;
					break;
				}
			}

			if (bFound)
				return (void *)pSearchStart;

			pSearchStart++;
		}
	}

	return NULL;
}

void *CMemoryUtils::FindPattern(HMODULE hModule, const char *pszPattern, unsigned int length, unsigned int offset /* = 0 */, char ignoreByte /* = '0x2A' */)
{
	if ( RetrieveModuleInfo(hModule, &m_ModuleInfo) )
	{
		unsigned char *pSearchStart = (unsigned char *)m_ModuleInfo.pBaseOfDll + offset;
		unsigned char *pSearchEnd = pSearchStart + m_ModuleInfo.SizeOfImage - length;

		while (pSearchStart < pSearchEnd)
		{
			bool bFound = true;

			for (register unsigned long i = 0; i < length; i++)
			{
				if (pszPattern[i] != ignoreByte && pszPattern[i] != pSearchStart[i])
				{
					bFound = false;
					break;
				}
			}

			if (bFound)
				return (void *)pSearchStart;

			pSearchStart++;
		}
	}

	return NULL;
}

void *CMemoryUtils::FindPattern(HMODULE hModule, unsigned char *pPattern, unsigned int length, unsigned int offset /* = 0 */, unsigned char ignoreByte /* = 0x2A */)
{
	if ( RetrieveModuleInfo(hModule, &m_ModuleInfo) )
	{
		unsigned char *pSearchStart = (unsigned char *)m_ModuleInfo.pBaseOfDll + offset;
		unsigned char *pSearchEnd = pSearchStart + m_ModuleInfo.SizeOfImage - length;

		while (pSearchStart < pSearchEnd)
		{
			bool bFound = true;

			for (register unsigned long i = 0; i < length; i++)
			{
				if (pPattern[i] != ignoreByte && pPattern[i] != pSearchStart[i])
				{
					bFound = false;
					break;
				}
			}

			if (bFound)
				return (void *)pSearchStart;

			pSearchStart++;
		}
	}

	return NULL;
}

void *CMemoryUtils::FindString(HMODULE hModule, const char *pszString, unsigned int offset /* = 0 */)
{
	if ( RetrieveModuleInfo(hModule, &m_ModuleInfo) )
	{
		unsigned long nLength = strlen(pszString);

		unsigned char *pSearchStart = (unsigned char *)m_ModuleInfo.pBaseOfDll + offset;
		unsigned char *pSearchEnd = pSearchStart + m_ModuleInfo.SizeOfImage - nLength;

		while (pSearchStart < pSearchEnd)
		{
			bool bFound = true;

			for (register unsigned long i = 0; i < nLength; i++)
			{
				if (pszString[i] != pSearchStart[i])
				{
					bFound = false;
					break;
				}
			}

			if (bFound)
				return (void *)pSearchStart;

			pSearchStart++;
		}
	}

	return NULL;
}

void *CMemoryUtils::FindAddress(HMODULE hModule, void *pAddress, unsigned int offset /* = 0 */)
{
	if ( RetrieveModuleInfo(hModule, &m_ModuleInfo) )
	{
		unsigned char *pSearchStart = (unsigned char *)m_ModuleInfo.pBaseOfDll + offset;
		unsigned char *pSearchEnd = pSearchStart + m_ModuleInfo.SizeOfImage - sizeof(void *);

		while (pSearchStart < pSearchEnd)
		{
			if (*(unsigned long *)pSearchStart == (unsigned long)pAddress)
				return (void *)pSearchStart;

			pSearchStart++;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Find a pattern within a range
//-----------------------------------------------------------------------------

void *CMemoryUtils::FindPatternWithin(HMODULE hModule, pattern_s *pPattern, void *pSearchStart, void *pSearchEnd)
{
	if ( RetrieveModuleInfo(hModule, &m_ModuleInfo) )
	{
		unsigned long nLength = pPattern->length;
		unsigned char *pSignature = &pPattern->signature;

		unsigned char *pModuleSearchStart = (unsigned char *)m_ModuleInfo.pBaseOfDll;
		unsigned char *pModuleSearchEnd = pModuleSearchStart + m_ModuleInfo.SizeOfImage - nLength;

		if (pModuleSearchStart > (unsigned char *)pSearchStart || pModuleSearchEnd < (unsigned char *)pSearchEnd)
			return NULL;

		pModuleSearchStart = (unsigned char *)pSearchStart;
		pModuleSearchEnd = (unsigned char *)pSearchEnd;

		while (pModuleSearchStart < pModuleSearchEnd)
		{
			bool bFound = true;

			for (register unsigned long i = 0; i < nLength; i++)
			{
				if (pSignature[i] != pPattern->ignorebyte && pSignature[i] != pModuleSearchStart[i])
				{
					bFound = false;
					break;
				}
			}

			if (bFound)
				return (void *)pModuleSearchStart;

			pModuleSearchStart++;
		}
	}

	return NULL;
}

void *CMemoryUtils::FindPatternWithin(HMODULE hModule, const char *pszPattern, char *pszMask, void *pSearchStart, void *pSearchEnd)
{
	if ( RetrieveModuleInfo(hModule, &m_ModuleInfo) )
	{
		unsigned long nMaskLength = strlen(pszMask);

		unsigned char *pModuleSearchStart = (unsigned char *)m_ModuleInfo.pBaseOfDll;
		unsigned char *pModuleSearchEnd = pModuleSearchStart + m_ModuleInfo.SizeOfImage - nMaskLength;

		if (pModuleSearchStart > (unsigned char *)pSearchStart || pModuleSearchEnd < (unsigned char *)pSearchEnd)
			return NULL;

		pModuleSearchStart = (unsigned char *)pSearchStart;
		pModuleSearchEnd = (unsigned char *)pSearchEnd;

		while (pModuleSearchStart < pModuleSearchEnd)
		{
			bool bFound = true;

			for (register unsigned long i = 0; i < nMaskLength; i++)
			{
				if (pszMask[i] != '?' && pszPattern[i] != pModuleSearchStart[i])
				{
					bFound = false;
					break;
				}
			}

			if (bFound)
				return (void *)pModuleSearchStart;

			pModuleSearchStart++;
		}
	}

	return NULL;
}

void *CMemoryUtils::FindPatternWithin(HMODULE hModule, const char *pszPattern, unsigned int length, void *pSearchStart, void *pSearchEnd, char ignoreByte /* = '0x2A' */)
{
	if ( RetrieveModuleInfo(hModule, &m_ModuleInfo) )
	{
		unsigned char *pModuleSearchStart = (unsigned char *)m_ModuleInfo.pBaseOfDll;
		unsigned char *pModuleSearchEnd = pModuleSearchStart + m_ModuleInfo.SizeOfImage - length;

		if (pModuleSearchStart > (unsigned char *)pSearchStart || pModuleSearchEnd < (unsigned char *)pSearchEnd)
			return NULL;

		pModuleSearchStart = (unsigned char *)pSearchStart;
		pModuleSearchEnd = (unsigned char *)pSearchEnd;

		while (pModuleSearchStart < pModuleSearchEnd)
		{
			bool bFound = true;

			for (register unsigned long i = 0; i < length; i++)
			{
				if (pszPattern[i] != ignoreByte && pszPattern[i] != pModuleSearchStart[i])
				{
					bFound = false;
					break;
				}
			}

			if (bFound)
				return (void *)pModuleSearchStart;

			pModuleSearchStart++;
		}
	}

	return NULL;
}

void *CMemoryUtils::FindPatternWithin(HMODULE hModule, unsigned char *pPattern, unsigned int length, void *pSearchStart, void *pSearchEnd, unsigned char ignoreByte /* = 0x2A */)
{
	if ( RetrieveModuleInfo(hModule, &m_ModuleInfo) )
	{
		unsigned char *pModuleSearchStart = (unsigned char *)m_ModuleInfo.pBaseOfDll;
		unsigned char *pModuleSearchEnd = pModuleSearchStart + m_ModuleInfo.SizeOfImage - length;

		if (pModuleSearchStart > (unsigned char *)pSearchStart || pModuleSearchEnd < (unsigned char *)pSearchEnd)
			return NULL;

		pModuleSearchStart = (unsigned char *)pSearchStart;
		pModuleSearchEnd = (unsigned char *)pSearchEnd;

		while (pModuleSearchStart < pModuleSearchEnd)
		{
			bool bFound = true;

			for (register unsigned long i = 0; i < length; i++)
			{
				if (pPattern[i] != ignoreByte && pPattern[i] != pModuleSearchStart[i])
				{
					bFound = false;
					break;
				}
			}

			if (bFound)
				return (void *)pModuleSearchStart;

			pModuleSearchStart++;
		}
	}

	return NULL;
}

void *CMemoryUtils::FindStringWithin(HMODULE hModule, const char *pszString, void *pSearchStart, void *pSearchEnd)
{
	if ( RetrieveModuleInfo(hModule, &m_ModuleInfo) )
	{
		unsigned long nLength = strlen(pszString);

		unsigned char *pModuleSearchStart = (unsigned char *)m_ModuleInfo.pBaseOfDll;
		unsigned char *pModuleSearchEnd = pModuleSearchStart + m_ModuleInfo.SizeOfImage - nLength;

		if (pModuleSearchStart > (unsigned char *)pSearchStart || pModuleSearchEnd < (unsigned char *)pSearchEnd)
			return NULL;

		pModuleSearchStart = (unsigned char *)pSearchStart;
		pModuleSearchEnd = (unsigned char *)pSearchEnd;

		while (pModuleSearchStart < pModuleSearchEnd)
		{
			bool bFound = true;

			for (register unsigned long i = 0; i < nLength; i++)
			{
				if (pszString[i] != pModuleSearchStart[i])
				{
					bFound = false;
					break;
				}
			}

			if (bFound)
				return (void *)pModuleSearchStart;

			pModuleSearchStart++;
		}
	}

	return NULL;
}

void *CMemoryUtils::FindAddressWithin(HMODULE hModule, void *pSearchStart, void *pSearchEnd, void *pAddress)
{
	if ( RetrieveModuleInfo(hModule, &m_ModuleInfo) )
	{
		unsigned char *pModuleSearchStart = (unsigned char *)m_ModuleInfo.pBaseOfDll;
		unsigned char *pModuleSearchEnd = pModuleSearchStart + m_ModuleInfo.SizeOfImage - sizeof(void *);

		if (pModuleSearchStart > (unsigned char *)pSearchStart || pModuleSearchEnd < (unsigned char *)pSearchEnd)
			return NULL;

		pModuleSearchStart = (unsigned char *)pSearchStart;
		pModuleSearchEnd = (unsigned char *)pSearchEnd;

		while (pModuleSearchStart < pModuleSearchEnd)
		{
			if (*(unsigned long *)pModuleSearchStart == (unsigned long)pAddress)
				return (void *)pModuleSearchStart;

			pModuleSearchStart++;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Export Memory API
//-----------------------------------------------------------------------------

CMemoryUtils g_MemoryUtils;

CMemoryUtils *MemoryUtils()
{
	return &g_MemoryUtils;
}