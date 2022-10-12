#ifndef AUTOBHOP_H
#define AUTOBHOP_H

#ifdef _WIN32
#pragma once
#endif

#define MAXCLIENTS ( 1 << 5 )

#include <eiface.h>

#include "memory_utils.h"
#include "detours_api.h"

class CBasePlayer;
class CBaseEntity;

class CAutoBunnyhop : public IServerPluginCallbacks
{
public:
	CAutoBunnyhop();

	// IServerPluginCallbacks methods
	virtual bool			Load( CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory );
	virtual void			Unload( void );
	virtual void			Pause( void );
	virtual void			UnPause( void );
	virtual const char		*GetPluginDescription( void );
	virtual void			LevelInit( char const *pMapName );
	virtual void			ServerActivate( edict_t *pEdictList, int edictCount, int clientMax );
	virtual void			GameFrame( bool simulating );
	virtual void			LevelShutdown( void );
	virtual void			ClientActive( edict_t *pEntity );
	virtual void			ClientDisconnect( edict_t *pEntity );
	virtual void			ClientPutInServer( edict_t *pEntity, char const *playername );
	virtual void			SetCommandClient( int index );
	virtual void			ClientSettingsChanged( edict_t *pEdict );
	virtual PLUGIN_RESULT	ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );
	virtual PLUGIN_RESULT	ClientCommand( edict_t *pEntity, const CCommand &args );
	virtual PLUGIN_RESULT	NetworkIDValidated( const char *pszUserName, const char *pszNetworkID );
	virtual void			OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue );

	// Version 3 of the interface
	virtual void			OnEdictAllocated( edict_t *edict );
	virtual void			OnEdictFreed( const edict_t *edict );

private:
	bool FindUserMessages( HMODULE hServerDLL, HMODULE hClientDLL );

public:
	void ResetBunnyhopState( bool bState );

	// Server
	bool IsAutoBunnyhopEnabled( int index ) const;
	bool IsAutoBunnyhopEnabled( CBaseEntity *pEntity ) const;
	void EnableAutoBunnyhop( edict_t *pPlayer, bool bState, bool bShowMessage );

	// Client
	bool Client_IsAutoBunnyhopEnabled( void ) const;
	void Client_EnableAutoBunnyhop( bool bState );

private:
	bool m_bClientAutoBunnyhop;
	bool m_bAutoBunnyhop[MAXCLIENTS + 1];

	DetourHandle_t m_hCheckJumpButtonServer;
	DetourHandle_t m_hCheckJumpButtonClient;
};

#endif // AUTOBHOP_H