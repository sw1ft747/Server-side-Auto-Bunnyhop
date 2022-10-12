#include "recipientfilter.h"
#include "utils.h"

CRecipientFilter::CRecipientFilter()
{
	m_bReliable = false;
	m_bInitMessage = false;
}

CRecipientFilter::~CRecipientFilter()
{
}

bool CRecipientFilter::IsReliable() const
{
	return m_bReliable;
}

bool CRecipientFilter::IsInitMessage() const
{
	return m_bInitMessage;
}

int CRecipientFilter::GetRecipientCount() const
{
	return m_Recipients.Count();
}

int	CRecipientFilter::GetRecipientIndex(int slot) const
{
	if ( slot < 0 || slot >= GetRecipientCount() )
		return -1;

	return m_Recipients[slot];
}

void CRecipientFilter::Reset()
{
	m_bReliable = false;
	m_bInitMessage = false;
	m_Recipients.RemoveAll();
}

void CRecipientFilter::MakeReliable()
{
	m_bReliable = true;
}

void CRecipientFilter::MakeInitMessage()
{
	m_bInitMessage = true;
}

void CRecipientFilter::AddAllPlayers()
{
	m_Recipients.RemoveAll();

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		edict_t *pPlayer = EntIndexToEdict(i);

		if ( !IsEdictValid(pPlayer) )
			continue;

		AddRecipient(pPlayer);
	}
}

void CRecipientFilter::AddRecipient(edict_t *pPlayer)
{
	if ( !IsEdictValid(pPlayer) )
		return;

	int index = EdictToEntIndex(pPlayer);

	if ( m_Recipients.Find(index) != m_Recipients.InvalidIndex() )
		return;

	m_Recipients.AddToTail(index);
}

void CRecipientFilter::RemoveAllRecipients()
{
	m_Recipients.RemoveAll();
}

void CRecipientFilter::RemoveRecipient(edict_t *pPlayer)
{
	int index = EdictToEntIndex(pPlayer);
	m_Recipients.FindAndRemove(index);
}

void CRecipientFilter::RemoveRecipientByPlayerIndex(int playerindex)
{
	Assert( playerindex > 0 && playerindex <= gpGlobals->maxClients );

	m_Recipients.FindAndRemove(playerindex);
}