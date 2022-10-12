#include "usermessages.h"

#define CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Force registration on .dll load
// FIXME:  Should this be a client/server system?
//-----------------------------------------------------------------------------
CUserMessages::CUserMessages()
{
	// Game specific registration function;
	//RegisterUserMessages();
}

CUserMessages::~CUserMessages()
{
	int c = m_UserMessages.Count();
	for ( int i = 0; i < c; ++i )
	{
		delete m_UserMessages[ i ];
	}
	m_UserMessages.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : int
//-----------------------------------------------------------------------------
int CUserMessages::LookupUserMessage( const char *name )
{
	int idx = m_UserMessages.Find( name );
	if ( idx == m_UserMessages.InvalidIndex() )
	{
		return -1;
	}

	return idx;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : int
//-----------------------------------------------------------------------------
int CUserMessages::GetUserMessageSize( int index )
{
	if ( index < 0 || index >= (int)m_UserMessages.Count() )
	{
		Error( "CUserMessages::GetUserMessageSize( %i ) out of range!!!\n", index );
	}

	CUserMessage *e = m_UserMessages[ index ];
	return e->size;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : char const
//-----------------------------------------------------------------------------
const char *CUserMessages::GetUserMessageName( int index )
{
	if ( index < 0 || index >= (int)m_UserMessages.Count() )
	{
		Error( "CUserMessages::GetUserMessageSize( %i ) out of range!!!\n", index );
	}

	return m_UserMessages.GetElementName( index );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CUserMessages::IsValidIndex( int index )
{
	return m_UserMessages.IsValidIndex( index );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//			size - -1 for variable size
//-----------------------------------------------------------------------------
void CUserMessages::Register( const char *name, int size )
{
	Assert( name );
	int idx = m_UserMessages.Find( name );
	if ( idx != m_UserMessages.InvalidIndex() )
	{
		Error( "CUserMessages::Register '%' already registered\n", name );
	}

	CUserMessage *entry = new CUserMessage;
	entry->size = size;
	entry->name = name;

	m_UserMessages.Insert( name, entry );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
void CUserMessages::Unregister( const char *name )
{
	Assert( name );
	int idx = m_UserMessages.Find( name );
	if ( idx == m_UserMessages.InvalidIndex() )
	{
		Error( "CUserMessages::Unregister '%' is not registered\n", name );
	}

	CUserMessage *entry = m_UserMessages.Element( idx );
	delete entry;

	m_UserMessages.RemoveAt( idx );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//			hook - 
//-----------------------------------------------------------------------------
void CUserMessages::HookMessage( const char *name, pfnUserMsgHook hook )
{
#if defined( CLIENT_DLL )
	Assert( name );
	Assert( hook );

	int idx = m_UserMessages.Find( name );
	if ( idx == m_UserMessages.InvalidIndex() )
	{
		DevMsg( "CUserMessages::HookMessage:  no such message %s\n", name );
		Assert( 0 );
		return;
	}

	int i = m_UserMessages[ idx ]->clienthooks.AddToTail();
	m_UserMessages[ idx ]->clienthooks[i] = hook;

#else
	Error( "CUserMessages::HookMessage called from server code!!!\n" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pszName - 
//			iSize - 
//			*pbuf - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CUserMessages::DispatchUserMessage( int msg_type, bf_read &msg_data )
{
#if defined( CLIENT_DLL )
	if ( msg_type < 0 || msg_type >= (int)m_UserMessages.Count() )
	{
		DevMsg( "CUserMessages::DispatchUserMessage:  Bogus msg type %i (max == %i)\n", msg_type, m_UserMessages.Count() );
		Assert( 0 );
		return false;
	}

	CUserMessage *entry = m_UserMessages[ msg_type ];

	if ( !entry )
	{
		DevMsg( "CUserMessages::DispatchUserMessage:  Missing client entry for msg type %i\n", msg_type );
		Assert( 0 );
		return false;
	}

	if ( entry->clienthooks.Count() == 0 )
	{
		DevMsg( "CUserMessages::DispatchUserMessage:  missing client hook for %s\n", GetUserMessageName(msg_type) );
		Assert( 0 );
		return false;
	}

	for (int i = 0; i < entry->clienthooks.Count(); i++  )
	{
		bf_read msg_copy = msg_data;

		pfnUserMsgHook hook = entry->clienthooks[i];
		(*hook)( msg_copy );
	}
	return true;
#else
	Error( "CUserMessages::DispatchUserMessage called from server code!!!\n" );
	return false;
#endif
}

//-----------------------------------------------------------------------------
// Message networking
//-----------------------------------------------------------------------------

bf_write *g_pMsgBuffer = NULL;
static CRecipientFilter s_pFilterBuffer;

const char *g_szUserMessages[] =
{
	"Geiger",
	"Train",
	"HudText",
	"SayText",
	"SayText2",
	"TextMsg",
	"HudMsg",
	"ResetHUD",
	"GameTitle",
	"ItemPickup",
	"ShowMenu",
	"Shake",
	"Fade",
	"VGUIMenu",
	"Rumble",
	"CloseCaption",
	"CloseCaptionDirect",
	"SendAudio",
	"RawAudio",
	"VoiceMask",
	"RequestState",
	"BarTime",
	"Damage",
	"RadioText",
	"HintText",
	"Key",
	"ReloadEffect",
	"PlayerAnimEvent",
	"AmmoDenied",
	"UpdateRadar",
	"KillCam",
	"MarkAchievement",
	"Splatter",
	"MeleeSlashSplatter",
	"MeleeClub",
	"MudSplatter",
	"SplatterClear",
	"MessageText",
	"TransitionRestore",
	"Spawn",
	"CreditsLine",
	"CreditsMsg",
	"JoinLateMsg",
	"StatsCrawlMsg",
	"StatsSkipState",
	"ShowStats",
	"BlurFade",
	"MusicCmd",
	"WitchBloodSplatter",
	"AchievementEvent",
	"PZDmgMsg",
	"AllPlayersConnectedGameStarting",
	"VoteRegistered",
	"DisconnectToLobby",
	"CallVoteFailed",
	"SteamWeaponStatData",
	"CurrentTimescale",
	"DesiredTimescale",
	"PZEndGamePanelMsg",
	"PZEndGamePanelVoteRegisteredMsg",
	"PZEndGameVoteStatsMsg",
	"VoteStart",
	"VotePass",
	"VoteFail"
};

void UserMessageBegin(UserMsgDest dest, const int nMsgType, const char *pszMessage, edict_t *pRecipient = NULL)
{
	switch (dest)
	{
	case MSG_UNICAST:
		s_pFilterBuffer = CSingleUserRecipientFilter(pRecipient);
		break;

	case MSG_UNICAST_RELIABLE:
		s_pFilterBuffer = CReliableSingleUserRecipientFilter(pRecipient);
		break;

	case MSG_BROADCAST:
		s_pFilterBuffer = CBroadcastRecipientFilter();
		break;

	case MSG_BROADCAST_RELIABLE:
		s_pFilterBuffer = CReliableBroadcastRecipientFilter();
		break;
	}

	g_pMsgBuffer = g_pEngineServer->UserMessageBegin(&s_pFilterBuffer, nMsgType, pszMessage);
}