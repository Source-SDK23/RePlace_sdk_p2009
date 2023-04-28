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
	CHandle<CBaseEntity> entity;
	MoveType_t moveType;
	SolidType_t solidType;
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

	void	PlacementThink(void);


	void	SetSlot(int slot); // Set camera slot
	void	ChangeScale(bool scaleUp); // Change placement scale
	int		GetState(void);

	typedef CUtlMap<int, CBaseEntity*> CMemMap;
private:
	CameraState		m_cameraState;
	bool	m_buttonPressed; // For input
	CUtlVector< CameraEntity* >	m_inventory;
	int		m_current_inventory_slot;

	DECLARE_DATADESC();
};

#endif // WEAPON_CAMERA_H