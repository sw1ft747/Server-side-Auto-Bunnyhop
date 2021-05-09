// C++
// Server-side Auto Bunnyhop

#include "autobhop.h"

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CAutoBunnyhop, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_AutoBunnyhop);

void OnConVarChange(IConVar *var, const char *pOldValue, float flOldValue)
{
	ConVar *cvar = dynamic_cast<ConVar *>(var);
	
	if (cvar->GetFloat() == flOldValue)
		return;

	if (cvar->GetBool())
	{
		if (!g_AutoBunnyhop.IsAutoBunnyhopEnabled())
			g_AutoBunnyhop.EnableAutoBunnyhop();
	}
	else if (g_AutoBunnyhop.IsAutoBunnyhopEnabled())
	{
		g_AutoBunnyhop.DisableAutoBunnyhop();
	}
}

CAutoBunnyhop::CAutoBunnyhop() : m_CheckJumpButtonServerPatch(nullptr), m_CheckJumpButtonClientPatch(nullptr), bAutoBunnyhopEnabled(false)
{
	
}

bool CAutoBunnyhop::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory)
{
	g_pCVar = reinterpret_cast<ICvar *>(interfaceFactory(CVAR_INTERFACE_VERSION, nullptr));

	if (g_pCVar == nullptr)
	{
		Warning("[Auto Bunnyhop] Failed to get CVar interface\n");
		return false;
	}

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

	EnableAutoBunnyhop();

	ConVar_Register();

	Msg("[Auto Bunnyhop] Successfully loaded\n");

	return true;
}

void CAutoBunnyhop::Unload(void)
{
	ConVar_Unregister();

	DisableAutoBunnyhop();

	delete m_CheckJumpButtonServerPatch;
	delete m_CheckJumpButtonClientPatch;

	Msg("[Auto Bunnyhop] Successfully unloaded\n");
}

void CAutoBunnyhop::Pause(void)
{
	
}

void CAutoBunnyhop::UnPause(void)
{
	
}

const char *CAutoBunnyhop::GetPluginDescription(void)
{
	return "Auto Bunnyhop v1.1 : Sw1ft";
}

void CAutoBunnyhop::LevelInit(char const *pMapName)
{

}

void CAutoBunnyhop::ServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
{

}

void CAutoBunnyhop::GameFrame(bool simulating)
{

}

void CAutoBunnyhop::LevelShutdown(void)
{

}

void CAutoBunnyhop::ClientActive(edict_t *pEntity)
{

}

void CAutoBunnyhop::ClientDisconnect(edict_t *pEntity)
{

}

void CAutoBunnyhop::ClientPutInServer(edict_t *pEntity, char const *playername)
{

}

void CAutoBunnyhop::SetCommandClient(int index)
{

}

void CAutoBunnyhop::ClientSettingsChanged(edict_t *pEdict)
{

}

PLUGIN_RESULT CAutoBunnyhop::ClientConnect(bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen)
{
	return PLUGIN_CONTINUE;
}

PLUGIN_RESULT CAutoBunnyhop::ClientCommand(edict_t *pEntity, const CCommand &args)
{
	return PLUGIN_CONTINUE;
}

PLUGIN_RESULT CAutoBunnyhop::NetworkIDValidated(const char *pszUserName, const char *pszNetworkID)
{
	return PLUGIN_CONTINUE;
}

void CAutoBunnyhop::OnQueryCvarValueFinished(QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue)
{

}

void CAutoBunnyhop::OnEdictAllocated(edict_t *edict)
{

}

void CAutoBunnyhop::OnEdictFreed(const edict_t *edict)
{

}

void CAutoBunnyhop::EnableAutoBunnyhop()
{
	m_CheckJumpButtonServerPatch->Patch();
	m_CheckJumpButtonClientPatch->Patch();

	bAutoBunnyhopEnabled = true;
}

void CAutoBunnyhop::DisableAutoBunnyhop()
{
	m_CheckJumpButtonServerPatch->Unpatch();
	m_CheckJumpButtonClientPatch->Unpatch();

	bAutoBunnyhopEnabled = false;
}

bool CAutoBunnyhop::IsAutoBunnyhopEnabled() const
{
	return bAutoBunnyhopEnabled;
}
