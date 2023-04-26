//=========                                                        ============//
//
// Purpose: 
//
//=============================================================================//

#include "basehlcombatweapon.h"
#include "soundenvelope.h"

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
private:
	CameraState		m_cameraState;
	bool	m_buttonPressed; // For input

	DECLARE_DATADESC();
};

#endif // WEAPON_CAMERA_H