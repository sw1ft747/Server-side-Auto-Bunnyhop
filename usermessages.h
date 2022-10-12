#ifndef USER_MESSAGES_H
#define USER_MESSAGES_H

#pragma warning(disable : 26812)

#include <eiface.h>
#include <utldict.h>
#include <utlvector.h>
#include <bitbuf.h>

#include "recipientfilter.h"

// Client dispatch function for usermessages
typedef void (*pfnUserMsgHook)(bf_read &msg);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CUserMessage
{
public:
	// byte size of message, or -1 for variable sized
	int				size;
	const char *name;
	// Client only dispatch function for message
	CUtlVector<pfnUserMsgHook>	clienthooks;
};

//-----------------------------------------------------------------------------
// Purpose: Interface for registering and dispatching usermessages
// Shred code creates same ordered list on client/server
//-----------------------------------------------------------------------------
class CUserMessages
{
public:

	CUserMessages();
	~CUserMessages();

	// Returns -1 if not found, otherwise, returns appropriate index
	int		LookupUserMessage(const char *name);
	int		GetUserMessageSize(int index);
	const char *GetUserMessageName(int index);
	bool	IsValidIndex(int index);

	// Server only
	void	Register(const char *name, int size);
	void	Unregister(const char *name);

	// Client only
	void	HookMessage(const char *name, pfnUserMsgHook hook);
	bool	DispatchUserMessage(int msg_type, bf_read &msg_data);

public:

	CUtlDict< CUserMessage *, int >	m_UserMessages;
};

extern CUserMessages *usermessages_server;
extern CUserMessages *usermessages_client;

//-----------------------------------------------------------------------------
// Message networking
//-----------------------------------------------------------------------------

enum UserMsgDest
{
	MSG_UNICAST = 0, // unreliable for a single client
	MSG_UNICAST_RELIABLE, // reliable for a single client
	MSG_BROADCAST, // unreliable for all clients
	MSG_BROADCAST_RELIABLE // reliable for all clients
};

enum UserMsgType
{
	MSG_TYPE_GEIGER = 0,
	MSG_TYPE_TRAIN,
	MSG_TYPE_HUDTEXT,
	MSG_TYPE_SAYTEXT,
	MSG_TYPE_SAYTEXT2,
	MSG_TYPE_TEXTMSG,
	MSG_TYPE_HUDMSG,
	MSG_TYPE_RESETHUD,
	MSG_TYPE_GAMETITLE,
	MSG_TYPE_ITEMPICKUP,
	MSG_TYPE_SHOWMENU,
	MSG_TYPE_SHAKE,
	MSG_TYPE_FADE,
	MSG_TYPE_VGUIMENU,
	MSG_TYPE_RUMBLE,
	MSG_TYPE_CLOSECAPTION,
	MSG_TYPE_CLOSECAPTIONDIRECT,
	MSG_TYPE_SENDAUDIO,
	MSG_TYPE_RAWAUDIO,
	MSG_TYPE_VOICEMASK,
	MSG_TYPE_REQUESTSTATE,
	MSG_TYPE_BARTIME,
	MSG_TYPE_DAMAGE,
	MSG_TYPE_RADIOTEXT,
	MSG_TYPE_HINTTEXT,
	MSG_TYPE_KEY,
	MSG_TYPE_RELOADEFFECT,
	MSG_TYPE_PLAYERANIMEVENT,
	MSG_TYPE_AMMODENIED,
	MSG_TYPE_UPDATERADAR,
	MSG_TYPE_KILLCAM,
	MSG_TYPE_MARKACHIEVEMENT,
	MSG_TYPE_SPLATTER,
	MSG_TYPE_MELEESLASHSPLATTER,
	MSG_TYPE_MELEECLUB,
	MSG_TYPE_MUDSPLATTER,
	MSG_TYPE_SPLATTERCLEAR,
	MSG_TYPE_MESSAGETEXT,
	MSG_TYPE_TRANSITIONRESTORE,
	MSG_TYPE_SPAWN,
	MSG_TYPE_CREDITSLINE,
	MSG_TYPE_CREDITSMSG,
	MSG_TYPE_JOINLATEMSG,
	MSG_TYPE_STATSCRAWLMSG,
	MSG_TYPE_STATSSKIPSTATE,
	MSG_TYPE_SHOWSTATS,
	MSG_TYPE_BLURFADE,
	MSG_TYPE_MUSICCMD,
	MSG_TYPE_WITCHBLOODSPLATTER,
	MSG_TYPE_ACHIEVEMENTEVENT,
	MSG_TYPE_PZDMGMSG,
	MSG_TYPE_ALLPLAYERSCONNECTEDGAMESTARTING,
	MSG_TYPE_VOTEREGISTERED,
	MSG_TYPE_DISCONNECTTOLOBBY,
	MSG_TYPE_CALLVOTEFAILED,
	MSG_TYPE_STEAMWEAPONSTATDATA,
	MSG_TYPE_CURRENTTIMESCALE,
	MSG_TYPE_DESIREDTIMESCALE,
	MSG_TYPE_PZENDGAMEPANELMSG,
	MSG_TYPE_PZENDGAMEPANELVOTEREGISTEREDMSG,
	MSG_TYPE_PZENDGAMEVOTESTATSMSG,
	MSG_TYPE_VOTESTART,
	MSG_TYPE_VOTEPASS,
	MSG_TYPE_VOTEFAIL
};

extern IVEngineServer *g_pEngineServer;
extern bf_write *g_pMsgBuffer;
extern const char *g_szUserMessages[];

void UserMessageBegin(UserMsgDest dest, const int nMsgType, const char *pszMessage, edict_t *pRecipient);

inline void MessageBegin(UserMsgDest dest, UserMsgType type, edict_t *pRecipient = NULL)
{
	UserMessageBegin(dest, static_cast<int>(type), g_szUserMessages[type], pRecipient);
}

inline void MessageBegin(UserMsgDest dest, int type, const char *pszName, edict_t *pRecipient = NULL)
{
	UserMessageBegin(dest, type, pszName, pRecipient);
}

inline void MessageBegin(IRecipientFilter *filter, UserMsgType type)
{
	g_pMsgBuffer = g_pEngineServer->UserMessageBegin(filter, static_cast<int>(type), g_szUserMessages[type]);
}

inline void MessageEnd()
{
	g_pEngineServer->MessageEnd();
	g_pMsgBuffer = NULL;
}

inline void WriteChar(int val)
{
	g_pMsgBuffer->WriteChar(val);
}

inline void WriteByte(int val)
{
	g_pMsgBuffer->WriteByte(val);
}

inline void WriteShort(int val)
{
	g_pMsgBuffer->WriteShort(val);
}

inline void WriteWord(int val)
{
	g_pMsgBuffer->WriteWord(val);
}

inline void WriteLong(long val)
{
	g_pMsgBuffer->WriteLong(val);
}

inline void WriteLongLong(int64 val)
{
	g_pMsgBuffer->WriteLongLong(val);
}

inline void WriteFloat(float val)
{
	g_pMsgBuffer->WriteFloat(val);
}

inline bool WriteBytes(const void *pBuf, int nBytes)
{
	return g_pMsgBuffer->WriteBytes(pBuf, nBytes);
}

inline bool WriteString(const char *pStr)
{
	return g_pMsgBuffer->WriteString(pStr);
}

#endif // USER_MESSAGES_H