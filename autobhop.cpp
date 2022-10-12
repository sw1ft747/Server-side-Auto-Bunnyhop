#pragma warning(disable : 26812)

#ifdef _WIN32
#include <Windows.h>
#endif

#include "autobhop.h"
#include "utils.h"
#include "usermessages.h"
#include <igamemovement.h>
#include <game/server/iplayerinfo.h>

// Solve incorrect linking
int (WINAPIV *__vsnprintf)(char *, size_t, const char *, va_list) = _vsnprintf;
int (WINAPIV *__vsnwprintf)(wchar_t *, size_t, const wchar_t *, ...) = _snwprintf;

void *GetInterfaceIteratively(CreateInterfaceFn interfaceFactory, const char *pszInterfaceVersion);

CGlobalVars *gpGlobals = NULL;
IVEngineServer *g_pEngineServer = NULL;
IPlayerInfoManager *g_pPlayerInfoManager = NULL;
ICvar *g_pCVar = NULL;

CUserMessages *usermessages_server = NULL;
CUserMessages *usermessages_client = NULL;

CAutoBunnyhop g_Autobunnyhop;

//-----------------------------------------------------------------------------
// CVars/ConCommands
//-----------------------------------------------------------------------------

ConVar sv_autobunnyhop("sv_autobunnyhop", "1", FCVAR_ARCHIVE | FCVAR_NOTIFY);

//-----------------------------------------------------------------------------
// Patterns
//-----------------------------------------------------------------------------

namespace Patterns
{
	DEFINE_PATTERN(UserMessageBegin, "55 8B EC 8B 0D ? ? ? ? 56 57"); // server's usermessages
	DEFINE_PATTERN(CHLClient__DispatchUserMessage, "55 8B EC 8B 0D ? ? ? ? 5D E9"); // client's usermessages
	DEFINE_PATTERN(CGameMovement__CheckJumpButton, "56 8B F1 8B 86 ? ? ? ? 85 C0 74 14 80 B8 ? ? ? ? 00 74 0B 8B 76 08 83 4E 28 02 32 C0 5E");
}

//-----------------------------------------------------------------------------
// Declare hooks
//-----------------------------------------------------------------------------

DECLARE_CLASS_HOOK(bool, CheckJumpButtonServer, IGameMovement *thisptr);
DECLARE_CLASS_HOOK(bool, CheckJumpButtonClient, IGameMovement *thisptr);

//-----------------------------------------------------------------------------
// CGameMovement::CheckJumpButton
//-----------------------------------------------------------------------------

DECLARE_CLASS_FUNC(bool, HOOKED_CheckJumpButtonServer, IGameMovement *thisptr)
{
	CBaseEntity *pPlayer = (CBaseEntity *)((uintptr_t *)thisptr)[1];

	CMoveData *mv = reinterpret_cast<CMoveData *>(((uintptr_t *)thisptr)[2]);
	const int IN_JUMP = mv->m_nOldButtons & (1 << 1);

	if ( g_Autobunnyhop.IsAutoBunnyhopEnabled(pPlayer) )
	{
		mv->m_nOldButtons &= ~IN_JUMP;
	}

	bool bJumped = ORIG_CheckJumpButtonServer(thisptr);

	mv->m_nOldButtons |= IN_JUMP;

	return bJumped;
}

DECLARE_CLASS_FUNC(bool, HOOKED_CheckJumpButtonClient, IGameMovement *thisptr)
{
	CMoveData *mv = reinterpret_cast<CMoveData *>(((uintptr_t *)thisptr)[2]);
	const int IN_JUMP = mv->m_nOldButtons & (1 << 1);

	if ( g_Autobunnyhop.Client_IsAutoBunnyhopEnabled() )
	{
		mv->m_nOldButtons &= ~IN_JUMP;
	}

	bool bJumped = ORIG_CheckJumpButtonClient(thisptr);

	mv->m_nOldButtons |= IN_JUMP;

	return bJumped;
}

//-----------------------------------------------------------------------------
// Process user message 'AutoBhop'
//-----------------------------------------------------------------------------

static void UserMsgHook_AutoBhop(bf_read &msg)
{
	int iType = msg.ReadByte();

	switch (iType)
	{
	case 0: // enable/disable
	{
		bool bEnable = !!msg.ReadByte();

		g_Autobunnyhop.Client_EnableAutoBunnyhop( bEnable );
		break;
	}

	default:
		Warning("UserMsgHook_AutoBhop: unrecognized type\n");
		break;
	}
}

//-----------------------------------------------------------------------------
// Process change of ConVar 'sv_autobunnyhop'
//-----------------------------------------------------------------------------

static void sv_autobunnyhop_ChangeCallback(IConVar *var, const char *pOldValue, float flOldValue)
{
	bool bEnable = sv_autobunnyhop.GetBool();

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		edict_t *pPlayer = EntIndexToEdict(i);

		if ( !IsEdictValid(pPlayer) )
			continue;

		g_Autobunnyhop.EnableAutoBunnyhop( pPlayer, bEnable, false );
	}
}

//-----------------------------------------------------------------------------
// Implement lookup stuff
//-----------------------------------------------------------------------------

bool CAutoBunnyhop::FindUserMessages(HMODULE hServerDLL, HMODULE hClientDLL)
{
	ud_t inst;

	void *pfnDispatchUserMessage = MemoryUtils()->FindPattern( hClientDLL, Patterns::CHLClient__DispatchUserMessage );

	if ( pfnDispatchUserMessage == NULL )
	{
		Warning("[Auto Bunnyhop] Failed to find function \"CHLClient::DispatchUserMessage\"\n");
		return false;
	}
	
	void *pfnUserMessageBegin = MemoryUtils()->FindPattern( hServerDLL, Patterns::UserMessageBegin );

	if ( pfnUserMessageBegin == NULL )
	{
		Warning("[Auto Bunnyhop] Failed to find function \"UserMessageBegin\"\n");
		return false;
	}

	MemoryUtils()->InitDisasm( &inst, pfnDispatchUserMessage, 32, 16 );

	while ( MemoryUtils()->Disassemble(&inst) )
	{
		if ( inst.mnemonic == UD_Imov && inst.operand[0].type == UD_OP_REG && inst.operand[0].base == UD_R_ECX && inst.operand[1].type == UD_OP_MEM )
		{
			usermessages_client = *reinterpret_cast<CUserMessages **>(inst.operand[1].lval.udword);
			break;
		}
	}
	
	MemoryUtils()->InitDisasm( &inst, pfnUserMessageBegin, 32, 16 );

	while ( MemoryUtils()->Disassemble(&inst) )
	{
		if ( inst.mnemonic == UD_Imov && inst.operand[0].type == UD_OP_REG && inst.operand[0].base == UD_R_ECX && inst.operand[1].type == UD_OP_MEM )
		{
			usermessages_server = *reinterpret_cast<CUserMessages **>(inst.operand[1].lval.udword);
			break;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Implement plugin methods
//-----------------------------------------------------------------------------

CAutoBunnyhop::CAutoBunnyhop()
{
	ResetBunnyhopState(true);

	m_bClientAutoBunnyhop = true;

	m_hCheckJumpButtonServer = 0;
	m_hCheckJumpButtonClient = 0;
}

bool CAutoBunnyhop::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory)
{
	static const char szVEngineServer[] = "VEngineServer022";
	static const char szPlayerInfoManager[] = "PlayerInfoManager002";
	static const char szCvar[] = "VEngineCvar007";

	void *pfnCheckJumpButtonServer, *pfnCheckJumpButtonClient;

	HMODULE hServerDLL = GetModuleHandle("server.dll");
	HMODULE hClientDLL = GetModuleHandle("client.dll");

	// Get general stuff

	g_pEngineServer = reinterpret_cast<IVEngineServer *>(GetInterfaceIteratively(interfaceFactory, szVEngineServer));

	if ( g_pEngineServer == NULL )
	{
		Warning("[Auto Bunnyhop] Failed to get interface \"IVEngineServer\"\n");
		return false;
	}

	g_pPlayerInfoManager = reinterpret_cast<IPlayerInfoManager *>(GetInterfaceIteratively(gameServerFactory, szPlayerInfoManager));

	if ( g_pPlayerInfoManager == NULL )
	{
		Warning("[Auto Bunnyhop] Failed to get interface \"IPlayerInfoManager\"\n");
		return false;
	}
	
	g_pCVar = reinterpret_cast<ICvar *>(GetInterfaceIteratively(interfaceFactory, szCvar));

	if ( g_pCVar == NULL )
	{
		Warning("[Auto Bunnyhop] Failed to get interface \"ICvar\"\n");
		return false;
	}

	gpGlobals = g_pPlayerInfoManager->GetGlobalVars();

	// Find CGameMovement::CheckJumpButton

	pfnCheckJumpButtonServer = MemoryUtils()->FindPattern( hServerDLL, Patterns::CGameMovement__CheckJumpButton );

	if ( pfnCheckJumpButtonServer == NULL )
	{
		Warning("[Auto Bunnyhop] Failed to find server's function \"CGameMovement::CheckJumpButton\"\n");
		return false;
	}

	pfnCheckJumpButtonClient = MemoryUtils()->FindPattern( hClientDLL, Patterns::CGameMovement__CheckJumpButton );
	
	if ( pfnCheckJumpButtonClient == NULL )
	{
		Warning("[Auto Bunnyhop] Failed to find client's function \"CGameMovement::CheckJumpButton\"\n");
		return false;
	}

	// Find server-side and client-side CUserMessages singletons

	if ( !FindUserMessages(hServerDLL, hClientDLL) )
	{
		return false;
	}

	// Detour CGameMovement::CheckJumpButton

	m_hCheckJumpButtonServer = DetoursAPI()->DetourFunction( pfnCheckJumpButtonServer, HOOKED_CheckJumpButtonServer, GET_FUNC_PTR(ORIG_CheckJumpButtonServer) );

	if ( m_hCheckJumpButtonServer == DETOUR_INVALID_HANDLE )
	{
		Warning("[Auto Bunnyhop] Failed to detour server's function \"CGameMovement::CheckJumpButton\"\n");
		return false;
	}
	
	m_hCheckJumpButtonClient = DetoursAPI()->DetourFunction( pfnCheckJumpButtonClient, HOOKED_CheckJumpButtonClient, GET_FUNC_PTR(ORIG_CheckJumpButtonClient) );

	if ( m_hCheckJumpButtonClient == DETOUR_INVALID_HANDLE )
	{
		Warning("[Auto Bunnyhop] Failed to detour client's function \"CGameMovement::CheckJumpButton\"\n");
		DetoursAPI()->RemoveDetour( m_hCheckJumpButtonServer );

		return false;
	}

	if ( usermessages_server->LookupUserMessage("AutoBhop") == -1 )
		usermessages_server->Register("AutoBhop", -1);

	if ( usermessages_client->LookupUserMessage("AutoBhop") == -1 )
		usermessages_client->Register("AutoBhop", -1);

	usermessages_client->HookMessage( "AutoBhop", UserMsgHook_AutoBhop );

	sv_autobunnyhop.InstallChangeCallback( sv_autobunnyhop_ChangeCallback );

	ConVar_Register();

	ConColorMsg({ 40, 255, 40, 255 }, "[Auto Bunnyhop] Successfully loaded\n");

	return true;
}

void CAutoBunnyhop::Unload(void)
{
	DetoursAPI()->RemoveDetour( m_hCheckJumpButtonServer );
	DetoursAPI()->RemoveDetour( m_hCheckJumpButtonClient );

	if ( usermessages_server->LookupUserMessage("AutoBhop") != -1 )
		usermessages_server->Unregister("AutoBhop");

	if ( usermessages_client->LookupUserMessage("AutoBhop") != -1 )
		usermessages_client->Unregister("AutoBhop");

	ConVar_Unregister();

	Msg("[Auto Bunnyhop] Successfully unloaded\n");
}

void CAutoBunnyhop::Pause(void)
{
	Warning("The plugin cannot be paused\n");
}

void CAutoBunnyhop::UnPause(void)
{
}

const char *CAutoBunnyhop::GetPluginDescription(void)
{
	return "Auto Bunnyhop v2.0.0 : Sw1ft";
}

void CAutoBunnyhop::LevelInit(char const *pMapName)
{
	ResetBunnyhopState( sv_autobunnyhop.GetBool() );
}

void CAutoBunnyhop::ServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
{
	ResetBunnyhopState( sv_autobunnyhop.GetBool() );
}

void CAutoBunnyhop::GameFrame(bool simulating)
{
}

void CAutoBunnyhop::LevelShutdown(void)
{
	ResetBunnyhopState( sv_autobunnyhop.GetBool() );
}

void CAutoBunnyhop::ClientActive(edict_t *pEntity)
{
}

void CAutoBunnyhop::ClientDisconnect(edict_t *pEntity)
{
	m_bAutoBunnyhop[ EdictToEntIndex(pEntity) ] = sv_autobunnyhop.GetBool();
}

void CAutoBunnyhop::ClientPutInServer(edict_t *pEntity, char const *playername)
{
	EnableAutoBunnyhop( pEntity, sv_autobunnyhop.GetBool(), false );
}

void CAutoBunnyhop::SetCommandClient(int index)
{
}

void CAutoBunnyhop::ClientSettingsChanged(edict_t *pEdict)
{
}

PLUGIN_RESULT CAutoBunnyhop::ClientConnect(bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen)
{
	m_bAutoBunnyhop[ EdictToEntIndex(pEntity) ] = sv_autobunnyhop.GetBool();

	return PLUGIN_CONTINUE;
}

PLUGIN_RESULT CAutoBunnyhop::ClientCommand(edict_t *pEntity, const CCommand &args)
{
	if ( !stricmp(args[0], "cl_autobunnyhop") ) // Toggle auto bunnyhop for client
	{
		if ( !sv_autobunnyhop.GetBool() )
			return PLUGIN_CONTINUE;

		bool bEnable;

		if ( args.ArgC() > 1 )
		{
			bEnable = !!atoi(args[1]);
		}
		else
		{
			bEnable = !m_bAutoBunnyhop[ EdictToEntIndex(pEntity) ];
		}

		EnableAutoBunnyhop( pEntity, bEnable, true );

		return PLUGIN_STOP;
	}

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

//-----------------------------------------------------------------------------
// Auto Bunnyhop implementation
//-----------------------------------------------------------------------------

void CAutoBunnyhop::ResetBunnyhopState(bool bState)
{
	memset( m_bAutoBunnyhop, bState, sizeof(m_bAutoBunnyhop) / sizeof(bool) );
}

bool CAutoBunnyhop::IsAutoBunnyhopEnabled(int index) const
{
	return m_bAutoBunnyhop[index];
}

bool CAutoBunnyhop::IsAutoBunnyhopEnabled(CBaseEntity *pEntity) const
{
	return m_bAutoBunnyhop[ EntIndexOfBaseEntity(pEntity) ];
}

void CAutoBunnyhop::EnableAutoBunnyhop(edict_t *pPlayer, bool bState, bool bShowMessage)
{
	MessageBegin( MSG_UNICAST_RELIABLE, usermessages_server->LookupUserMessage("AutoBhop"), "AutoBhop", pPlayer );
		WriteByte( 0 );
		WriteByte( bState ? 1 : 0 );
	MessageEnd();

	if ( bShowMessage )
	{
		MessageBegin( MSG_UNICAST_RELIABLE, MSG_TYPE_TEXTMSG, pPlayer );
			WriteByte( 3 ); // destination: chat
			WriteString( bState ? "\x05>\x04 Auto Bunnyhop is \x03ON" : "\x05>\x04 Auto Bunnyhop is \x03OFF" );
			WriteString( "" );
			WriteString( "" );
			WriteString( "" );
			WriteString( "" );
		MessageEnd();
	}

	m_bAutoBunnyhop[ EdictToEntIndex(pPlayer) ] = bState;
}

bool CAutoBunnyhop::Client_IsAutoBunnyhopEnabled(void) const
{
	return m_bClientAutoBunnyhop;
}

void CAutoBunnyhop::Client_EnableAutoBunnyhop(bool bState)
{
	m_bClientAutoBunnyhop = bState;
}

//-----------------------------------------------------------------------------
// iface
//-----------------------------------------------------------------------------

void *GetInterfaceIteratively(CreateInterfaceFn interfaceFactory, const char *pszInterfaceVersion)
{
	void *pInterface = NULL;
	char *szInterfaceVersion = const_cast<char *>(pszInterfaceVersion);

	const size_t length = strlen(szInterfaceVersion), last_idx = length - 1, post_last_idx = length - 2;

	while (szInterfaceVersion[post_last_idx] != '0' || szInterfaceVersion[last_idx] != '0')
	{
		if ( pInterface = interfaceFactory(szInterfaceVersion, NULL) )
			return pInterface;

		if ( szInterfaceVersion[last_idx] == '0' )
		{
			szInterfaceVersion[last_idx] = '9';

			if (szInterfaceVersion[post_last_idx] != '0')
				--szInterfaceVersion[post_last_idx];
		}
		else
		{
			--szInterfaceVersion[last_idx];
		}
	}

	return pInterface;
}

//-----------------------------------------------------------------------------
// Expose singleton
//-----------------------------------------------------------------------------

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CAutoBunnyhop, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_Autobunnyhop);