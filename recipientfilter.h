#ifndef RECIPIENTFILTER_H
#define RECIPIENTFILTER_H

#include <edict.h>

#include "irecipientfilter.h"

class CRecipientFilter : public IRecipientFilter
{
public:
	CRecipientFilter();
	virtual 		~CRecipientFilter();

	virtual bool	IsReliable() const;
	virtual bool	IsInitMessage() const;

	virtual int		GetRecipientCount() const;
	virtual int		GetRecipientIndex(int slot) const;

public:
	void			Reset();

	void			MakeInitMessage();
	void			MakeReliable();

	void			AddAllPlayers();
	void			AddRecipient(edict_t *pPlayer);
	void			RemoveAllRecipients();
	void			RemoveRecipient(edict_t *pPlayer);
	void			RemoveRecipientByPlayerIndex(int playerindex);

protected:
	bool				m_bReliable;
	bool				m_bInitMessage;
	CUtlVector<int>		m_Recipients;
};

//-----------------------------------------------------------------------------
// Filter for a single player (unreliable)
//-----------------------------------------------------------------------------

class CSingleUserRecipientFilter : public CRecipientFilter
{
public:
	CSingleUserRecipientFilter(edict_t *pPlayer)
	{
		AddRecipient(pPlayer);
	}
};

//-----------------------------------------------------------------------------
// Filter for a single player (reliable)
//-----------------------------------------------------------------------------

class CReliableSingleUserRecipientFilter : public CRecipientFilter
{
public:
	CReliableSingleUserRecipientFilter(edict_t *pPlayer)
	{
		AddRecipient(pPlayer);
		MakeReliable();
	}
};

//-----------------------------------------------------------------------------
// Filter for all players (unreliable)
//-----------------------------------------------------------------------------

class CBroadcastRecipientFilter : public CRecipientFilter
{
public:
	CBroadcastRecipientFilter()
	{
		AddAllPlayers();
	}
};

//-----------------------------------------------------------------------------
// Filter for all players (reliable)
//-----------------------------------------------------------------------------

class CReliableBroadcastRecipientFilter : public CBroadcastRecipientFilter
{
public:
	CReliableBroadcastRecipientFilter()
	{
		MakeReliable();
	}
};

#endif // RECIPIENTFILTER_H