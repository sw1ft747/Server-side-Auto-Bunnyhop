#ifndef UTILS_H
#define UTILS_H

#include <icliententity.h>
#include <edict.h>

#include "memory_utils.h"

extern CGlobalVars *gpGlobals;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

inline bool IsEdictValid(const edict_t *pEdict)
{
	return pEdict != NULL && ((edict_t *)pEdict)->GetUnknown();
}

inline int EdictToEntIndex(const edict_t *pEdict)
{
	return (int)(pEdict - gpGlobals->pEdicts);
}

inline CBaseEntity *EdictToBaseEntity(edict_t *pEdict)
{
	return pEdict->GetUnknown()->GetBaseEntity();
}

inline edict_t *EntIndexToEdict(const int nIndex)
{
	if ( nIndex > 0 && nIndex <= gpGlobals->maxEntities )
		return (edict_t *)(gpGlobals->pEdicts + nIndex);

	return NULL;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

inline edict_t *BaseEntityToEdict(CBaseEntity *pEntity)
{
	IServerUnknown *pUnknown = reinterpret_cast<IServerUnknown *>(pEntity);
	IServerNetworkable *pNetworkable = pUnknown->GetNetworkable();

	if ( pNetworkable != NULL )
		return pNetworkable->GetEdict();

	return NULL;
}

inline int EntIndexOfBaseEntity(CBaseEntity *pEntity) // server
{
	IServerUnknown *pUnknown = reinterpret_cast<IServerUnknown *>(pEntity);
	CBaseHandle handle = pUnknown->GetRefEHandle();

	if ( handle.GetEntryIndex() >= MAX_EDICTS )
		return handle.ToInt() | (1 << 31);
	else
		return handle.GetEntryIndex();
}

inline int EntIndexOfBaseEntity(IClientEntity *pEntity) // client
{
	IClientNetworkable *pNetworkable = pEntity->GetClientNetworkable();

	// IClientNetworkable::entindex
	int (__thiscall *entindex)(IClientNetworkable *) = (int (__thiscall *)(IClientNetworkable *))MemoryUtils()->GetVirtualFunction(pNetworkable, 8); // Fix SDK

	if ( pNetworkable != NULL )
		return entindex(pNetworkable);

	return -1;
}

inline CBaseEntity *HandleToBaseEntity(CBaseHandle handle)
{
	int nIndex;

	if ( !handle.IsValid() )
		return NULL;

	if ( handle.GetEntryIndex() >= MAX_EDICTS )
		nIndex = handle.ToInt() | (1 << 31);
	else
		nIndex = handle.GetEntryIndex();

	edict_t *pEdict = EntIndexToEdict(nIndex);

	if ( IsEdictValid(pEdict) )
		return EdictToBaseEntity(pEdict);

	return NULL;
}

#endif // UTILS_H