// Copypasta from SvenMod

#ifndef DETOURS_API_H
#define DETOURS_API_H

#ifdef _WIN32
#pragma once
#endif

#include <vector>

#include "hashtable.h"
#include "memory_utils.h"

//-----------------------------------------------------------------------------
// Macro definitions
//-----------------------------------------------------------------------------

#define FUNC_SIGNATURE(retType, callConv, typeName, ...) typedef retType(callConv *typeName)(__VA_ARGS__)

#define DECLARE_FUNC_PTR(retType, callConv, funcName, ...) typedef retType(callConv *(funcName##Fn))(__VA_ARGS__); funcName##Fn funcName = 0
#define DECLARE_FUNC(retType, callConv, funcName, ...) retType callConv funcName(__VA_ARGS__)

#define DECLARE_CLASS_FUNC(retType, funcName, thisPtr, ...) retType __fastcall funcName(thisPtr, void *edx, __VA_ARGS__)

#define GET_FUNC_PTR(funcName) (void **)&funcName

#define DECLARE_HOOK(retType, callConv, funcName, ...) \
	typedef retType(callConv *funcName##Fn)(__VA_ARGS__); \
	static retType callConv HOOKED_##funcName(__VA_ARGS__); \
	funcName##Fn ORIG_##funcName = 0

#define DECLARE_CLASS_HOOK(retType, funcName, thisPtr, ...) \
	typedef retType(__thiscall *funcName##Fn)(thisPtr, __VA_ARGS__); \
	static retType __fastcall HOOKED_##funcName(thisPtr, void *edx, __VA_ARGS__); \
	funcName##Fn ORIG_##funcName = 0

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------

class CDetourFunction;
class CDetourContext;

//-----------------------------------------------------------------------------
// Detour handle
//-----------------------------------------------------------------------------

typedef int DetourHandle_t;
#define DETOUR_INVALID_HANDLE (DetourHandle_t)(-1)

//-----------------------------------------------------------------------------
// Type
//-----------------------------------------------------------------------------

typedef enum
{
	DETOUR_FUNCTION = 0,
	DETOUR_VTABLE_FUNCTION,
	DETOUR_IAT_FUNCTION
} DETOUR_TYPE;

//-----------------------------------------------------------------------------
// CDetoursAPI
//-----------------------------------------------------------------------------

class CDetoursAPI
{
	friend class CDetourContext;

public:
	CDetoursAPI();
	~CDetoursAPI();

	void Init();

	//-----------------------------------------------------------------------------
	// Hook function
	// 
	// @param ppOriginalFunction - pointer to pointer to original function to call
	// @param iDisasmMinBytes - minimum bytes to steal from original function, it
	// can be helpful if detour's trampoline crashes (i.e. jump-short opcode causes it)
	// ToDo: fix it..
	//-----------------------------------------------------------------------------

	DetourHandle_t DetourFunction(void *pFunction, void *pDetourFunction, void **ppOriginalFunction, bool bPause = false, int iDisasmMinBytes = 5);

	//-----------------------------------------------------------------------------
	// Find function from import address table (using GetProcAddress) and hook it
	//-----------------------------------------------------------------------------

	DetourHandle_t DetourFunctionByName(HMODULE hModule, const char *pszFunctionName, void *pDetourFunction, void **ppOriginalFunction, bool bPause = false, int iDisasmMinBytes = 5);

	//-----------------------------------------------------------------------------
	// Hook function from virtual methods table
	//-----------------------------------------------------------------------------

	DetourHandle_t DetourVirtualFunction(void *pClassInstance, int nFunctionIndex, void *pDetourFunction, void **ppOriginalFunction, bool bPause = false);

	//-----------------------------------------------------------------------------
	// Pause function. Returns true if success
	//-----------------------------------------------------------------------------

	bool PauseDetour(DetourHandle_t hDetour);

	//-----------------------------------------------------------------------------
	// Unpause function. Returns true if success
	//-----------------------------------------------------------------------------

	bool UnpauseDetour(DetourHandle_t hDetour);

	//-----------------------------------------------------------------------------
	// Remove/unhook function
	//-----------------------------------------------------------------------------

	bool RemoveDetour(DetourHandle_t hDetour);

	//-----------------------------------------------------------------------------
	// Pause all registered detours
	//-----------------------------------------------------------------------------

	bool PauseAllDetours();

	//-----------------------------------------------------------------------------
	// Unpause all registered detours
	//-----------------------------------------------------------------------------

	bool UnpauseAllDetours();

private:
	DetourHandle_t CreateDetour(DETOUR_TYPE type, void *pFunction, void *pDetourFunction, void **ppOriginalFunction, bool bPause, int iDisasmMinBytes);
	DetourHandle_t AllocateDetourHandle();

	void SuspendThreads();
	void ResumeThreads();

	CHashTable<DetourHandle_t, CDetourFunction *> m_DetoursTable;
	CHashTable<void *, CDetourContext *> m_ContextsTable;

	std::vector<unsigned long> m_SuspendedThreads;
	unsigned long m_dwCurrentThreadID;
	unsigned long m_dwCurrentProcessID;

	int m_iDetourHandles;
};

extern CDetoursAPI g_DetoursAPI;
CDetoursAPI *DetoursAPI();

#endif // DETOURS_API_H