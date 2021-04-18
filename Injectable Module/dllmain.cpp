// C++
// Server-side Auto Bunnyhop

#include "patcher.h"
#include "signature_scanner.h"

namespace Signatures
{
	BYTE CheckJumpButton[] = { 0x84, 0x58, 0x28, 0x75, 0xF3, 0x6A, 0x00, 0x8B, 0xCE, 0xE8 };

	namespace Masks
	{
		const char CheckJumpButton[] = "xxxxxxxxxx";
	}
}

DWORD WINAPI MainThread(HMODULE hModule)
{
	DWORD dwServerDLL = (DWORD)GetModuleHandle(L"server.dll");
	DWORD dwClientDLL = (DWORD)GetModuleHandle(L"client.dll");

	if (dwClientDLL && dwServerDLL)
	{
		const BYTE pPatchedBytes[5] = { 0x90, 0x90, 0x90, 0x90, 0x90 };

		void *pCheckJumpButtonServer = FindPattern(L"server.dll", Signatures::CheckJumpButton, Signatures::Masks::CheckJumpButton);
		void *pCheckJumpButtonClient = FindPattern(L"client.dll", Signatures::CheckJumpButton, Signatures::Masks::CheckJumpButton);

		if (pCheckJumpButtonServer != nullptr && pCheckJumpButtonClient != nullptr)
		{
			Beep(500, 250);

			CPatcher *CheckJumpButtonServerPatch = new CPatcher(pCheckJumpButtonServer, pPatchedBytes, 5);
			CPatcher *CheckJumpButtonClientPatch = new CPatcher(pCheckJumpButtonClient, pPatchedBytes, 5);

			CheckJumpButtonServerPatch->Patch();
			CheckJumpButtonClientPatch->Patch();

			while (true)
			{
				if (GetAsyncKeyState(VK_END))
				{
					CheckJumpButtonServerPatch->Unpatch();
					CheckJumpButtonClientPatch->Unpatch();

					delete CheckJumpButtonServerPatch;
					delete CheckJumpButtonClientPatch;

					goto SUCCESS_EXIT;
				}

				Sleep(100);
			}
		}
	}

	Beep(500, 250);

SUCCESS_EXIT:
	Beep(500, 250);
	FreeLibraryAndExitThread(hModule, 0);
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		HANDLE hThread = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, nullptr);
		if (hThread) CloseHandle(hThread);
	}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}