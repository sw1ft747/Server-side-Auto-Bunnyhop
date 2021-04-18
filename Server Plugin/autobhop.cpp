// C++
// Server-side Auto Bunnyhop

#include "eiface.h"
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

private:
	CPatcher *m_CheckJumpButtonServerPatch;
	CPatcher *m_CheckJumpButtonClientPatch;
};

CAutoBunnyhop g_AutoBunnyhop;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CAutoBunnyhop, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_AutoBunnyhop);

//---------------------------------------------------------------------------------------------------
// Constructor
//---------------------------------------------------------------------------------------------------
CAutoBunnyhop::CAutoBunnyhop() : m_CheckJumpButtonServerPatch(nullptr), m_CheckJumpButtonClientPatch(nullptr)
{

}

//---------------------------------------------------------------------------------------------------
// Called when the plugin is loaded by the engine
//---------------------------------------------------------------------------------------------------
bool CAutoBunnyhop::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory)
{
	const BYTE pPatchedBytes[5] = { 0x90, 0x90, 0x90, 0x90, 0x90 };

	void *pCheckJumpButtonServer = FindPattern(L"server.dll", Signatures::CheckJumpButton, Signatures::Masks::CheckJumpButton);
	void *pCheckJumpButtonClient = FindPattern(L"client.dll", Signatures::CheckJumpButton, Signatures::Masks::CheckJumpButton);

	if (pCheckJumpButtonServer == nullptr || pCheckJumpButtonClient == nullptr)
	{
		Warning("[Auto Bunnyhop] Failed to get 'CGameMovement::CheckJumpButton' function\n");
		return false;
	}

	// NOP conditional | if ( mv->m_nOldButtons & IN_JUMP ) return; |
	m_CheckJumpButtonServerPatch = new CPatcher(pCheckJumpButtonServer, pPatchedBytes, 5);
	m_CheckJumpButtonClientPatch = new CPatcher(pCheckJumpButtonClient, pPatchedBytes, 5);

	m_CheckJumpButtonServerPatch->Patch();
	m_CheckJumpButtonClientPatch->Patch();

	Msg("[Auto Bunnyhop] Successfully loaded\n");

	return true;
}

//---------------------------------------------------------------------------------------------------
// Called when a plugin is unloaded
//---------------------------------------------------------------------------------------------------
void CAutoBunnyhop::Unload(void)
{
	m_CheckJumpButtonServerPatch->Unpatch();
	m_CheckJumpButtonClientPatch->Unpatch();

	delete m_CheckJumpButtonServerPatch;
	delete m_CheckJumpButtonClientPatch;

	Msg("[Auto Bunnyhop] Successfully unloaded\n");
}

//---------------------------------------------------------------------------------------------------
// Called when the operation of the plugin is paused
//---------------------------------------------------------------------------------------------------
void CAutoBunnyhop::Pause(void)
{
	m_CheckJumpButtonServerPatch->Unpatch();
	m_CheckJumpButtonClientPatch->Unpatch();
}

//---------------------------------------------------------------------------------------------------
// Called when a plugin is brought out of the paused state
//---------------------------------------------------------------------------------------------------
void CAutoBunnyhop::UnPause(void)
{
	m_CheckJumpButtonServerPatch->Patch();
	m_CheckJumpButtonClientPatch->Patch();
}

//---------------------------------------------------------------------------------------------------
// The name of this plugin, returned in "plugin_print" command
//---------------------------------------------------------------------------------------------------
const char *CAutoBunnyhop::GetPluginDescription(void)
{
	return "Auto Bunnyhop v1.0 : Sw1ft";
}

//---------------------------------------------------------------------------------------------------
// Called on level (map) startup, it's the first function called as a server enters a new level
//---------------------------------------------------------------------------------------------------
void CAutoBunnyhop::LevelInit(char const *pMapName)
{

}

//---------------------------------------------------------------------------------------------------
// Called when the server successfully enters a new level, this will happen after the LevelInit call
//---------------------------------------------------------------------------------------------------
void CAutoBunnyhop::ServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
{

}

//---------------------------------------------------------------------------------------------------
// Called once per server frame
//---------------------------------------------------------------------------------------------------
void CAutoBunnyhop::GameFrame(bool simulating)
{

}

//---------------------------------------------------------------------------------------------------
// Called when a server is changing to a new level or is being shutdown
//---------------------------------------------------------------------------------------------------
void CAutoBunnyhop::LevelShutdown(void) // can be called multiple times during a map change
{

}

//---------------------------------------------------------------------------------------------------
// Called after a client is fully spawned
//---------------------------------------------------------------------------------------------------
void CAutoBunnyhop::ClientActive(edict_t *pEntity)
{

}

//---------------------------------------------------------------------------------------------------
// Called when a client disconnects from the server
//---------------------------------------------------------------------------------------------------
void CAutoBunnyhop::ClientDisconnect(edict_t *pEntity)
{

}

//---------------------------------------------------------------------------------------------------
// Called when a client spawns into a server
//---------------------------------------------------------------------------------------------------
void CAutoBunnyhop::ClientPutInServer(edict_t *pEntity, char const *playername)
{

}

//---------------------------------------------------------------------------------------------------
// Called by the ConVar code to track of which client is entering a ConCommand
//---------------------------------------------------------------------------------------------------
void CAutoBunnyhop::SetCommandClient(int index)
{

}

//---------------------------------------------------------------------------------------------------
// Called when player specific cvars about a player change (for example the users name)
//---------------------------------------------------------------------------------------------------
void CAutoBunnyhop::ClientSettingsChanged(edict_t *pEdict)
{

}

//---------------------------------------------------------------------------------------------------
// Called when a client initially connects to a server
//---------------------------------------------------------------------------------------------------
PLUGIN_RESULT CAutoBunnyhop::ClientConnect(bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen)
{
	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------------------------
// Called when a remote client enters a command into the console
//---------------------------------------------------------------------------------------------------
PLUGIN_RESULT CAutoBunnyhop::ClientCommand(edict_t *pEntity, const CCommand &args)
{
	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------------------------
// Called when the server retrieves a clients network ID (i.e Steam ID)
//---------------------------------------------------------------------------------------------------
PLUGIN_RESULT CAutoBunnyhop::NetworkIDValidated(const char *pszUserName, const char *pszNetworkID)
{
	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------------------------
// Called when a query from IServerPluginHelpers::StartQueryCvarValue() finishes
//---------------------------------------------------------------------------------------------------
void CAutoBunnyhop::OnQueryCvarValueFinished(QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue)
{

}

//---------------------------------------------------------------------------------------------------
// ToDo
//---------------------------------------------------------------------------------------------------
void CAutoBunnyhop::OnEdictAllocated(edict_t *edict)
{

}

//---------------------------------------------------------------------------------------------------
// ToDo
//---------------------------------------------------------------------------------------------------
void CAutoBunnyhop::OnEdictFreed(const edict_t *edict)
{

}