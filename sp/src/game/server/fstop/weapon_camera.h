//=========                                                        ============//
//
// Purpose: 
//
//=============================================================================//

#include "basehlcombatweapon.h"
#include "soundenvelope.h"
#include "utlmap.h"

#ifndef WEAPON_CAMERA_H
#define WEAPON_CAMERA_H
#ifdef _WIN32
#pragma once
#endif

enum CameraState {
	CAMERA_NORMAL = 0,
	CAMERA_ZOOM,
	CAMERA_PLACEMENT
};

struct CameraEntity {
	DECLARE_SIMPLE_DATADESC();

	CHandle<CBaseEntity> entity;
	int moveType;
	int solidType;
	int effects;
	double modelRadius;
	double modelHeight;
};

//---------------------
// Camera
//---------------------
class CWeaponCamera:public CBaseHLCombatWeapon
{
public:
	DECLARE_CLASS( CWeaponCamera, CBaseHLCombatWeapon );
	DECLARE_SERVERCLASS();

	CWeaponCamera();

	void	Precache( void );
	void	PrimaryAttack( void );
	void	SecondaryAttack(void);
	bool	Reload(void);

	int		CapabilitiesGet(void) { return bits_CAP_WEAPON_RANGE_ATTACK1; };

	void	ItemPostFrame(void);

	typedef CUtlMap<int, CBaseEntity*> CMemMap;
private:
	CameraState		m_cameraState;
	bool	m_buttonPressed; // For input
	CUtlVector< CameraEntity* >	m_inventory;
	int		m_current_inventory_slot;

	DECLARE_DATADESC();
};

#endif // WEAPON_CAMERA_H