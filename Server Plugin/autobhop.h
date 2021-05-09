// C++
// Server-side Auto Bunnyhop

#pragma once

#undef _UNICODE

#include <Windows.h>

#include "eiface.h"
#include "icvar.h"
#include "iconvar.h"
#include "convar.h"

#include "patcher.h"
#include "signature_scanner.h"

// solve incorrect linking
int (WINAPIV *__vsnprintf)(char *, size_t, const char *, va_list) = _vsnprintf;
int (WINAPIV *__vsnwprintf)(wchar_t *, size_t, const wchar_t *, ...) = _snwprintf;

namespace Signatures
{
	BYTE CheckJumpButton[] = { 0x84, 0x58, 0x28, 0x75, 0xF3, 0x6A, 0x00, 0x8B, 0xCE, 0xE8 };

	namespace Masks
	{
		const char CheckJumpButton[] = "xxxxxxxxxx";
	}
}

void OnConVarChange(IConVar *var, const char *pOldValue, float flOldValue);

ConVar sv_autobunnyhopping("sv_autobunnyhopping",
						   "1",
						   FCVAR_RELEASE | FCVAR_REPLICATED,
						   "Players automatically re-jump while holding jump button",
						   true, 0.0f, true, 1.0f,
						   OnConVarChange);

class CAutoBunnyhop : public IServerPluginCallbacks
{
public:
	CAutoBunnyhop();

	// IServerPluginCallbacks methods
	virtual bool			Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory);
	virtual void			Unload(void);
	virtual void			Pause(void);
	virtual void			UnPause(void);
	virtual const char *GetPluginDescription(void);
	virtual void			LevelInit(char const *pMapName);
	virtual void			ServerActivate(edict_t *pEdictList, int edictCount, int clientMax);
	virtual void			GameFrame(bool simulating);
	virtual void			LevelShutdown(void);
	virtual void			ClientActive(edict_t *pEntity);
	virtual void			ClientDisconnect(edict_t *pEntity);
	virtual void			ClientPutInServer(edict_t *pEntity, char const *playername);
	virtual void			SetCommandClient(int index);
	virtual void			ClientSettingsChanged(edict_t *pEdict);
	virtual PLUGIN_RESULT	ClientConnect(bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen);
	virtual PLUGIN_RESULT	ClientCommand(edict_t *pEntity, const CCommand &args);
	virtual PLUGIN_RESULT	NetworkIDValidated(const char *pszUserName, const char *pszNetworkID);
	virtual void			OnQueryCvarValueFinished(QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue);

	// Version 3 of the interface
	virtual void			OnEdictAllocated(edict_t *edict);
	virtual void			OnEdictFreed(const edict_t *edict);

	void EnableAutoBunnyhop();
	void DisableAutoBunnyhop();
	bool IsAutoBunnyhopEnabled() const;

private:
	CPatcher *m_CheckJumpButtonServerPatch;
	CPatcher *m_CheckJumpButtonClientPatch;
	bool bAutoBunnyhopEnabled;
} g_AutoBunnyhop;
