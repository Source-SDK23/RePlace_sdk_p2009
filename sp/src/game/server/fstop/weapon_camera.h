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

class CameraEntity
{
public:

	DECLARE_SIMPLE_DATADESC();

	void			SetEntity(CBaseEntity* pEntity) { entity = pEntity; }
	CBaseEntity*	GetEntity(void) { return entity.Get(); }

	void			SetMoveType(MoveType_t pMoveType) { moveType = pMoveType; }
	MoveType_t		GetMoveType(void) { return moveType; }

	void			SetSolidType(SolidType_t pSolidType) { solidType = pSolidType; }
	SolidType_t		GetSolidType(void) { return solidType; }

	void			SetEffects(int pEffects) { effects = pEffects; }
	int				GetEffects(void) { return moveType; }

	void			SetShouldWake(bool pShouldWake ) { shouldWake = pShouldWake; }
	bool			GetShouldWake(void) { return shouldWake; }

	void			SetModelRadius(float pModelRadius) { modelRadius = pModelRadius; }
	float			GetModelRadius(void) { return modelRadius; }

	void			SetModelHeight(float pModelHeight) { modelHeight = pModelHeight; }
	float			GetModelHeight(void) { return modelHeight; }


	void			CaptureEntity(void);
	void			RestoreEntity(bool solidify=true);
	void			HideEntity(void);

private:
	EHANDLE			entity;
	MoveType_t		moveType;
	SolidType_t		solidType;
	int				effects;
	bool			shouldWake;

	float			modelRadius;
	float			modelHeight;
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

	void	SetZoom(bool zoomState);

	typedef CUtlMap<int, CBaseEntity*> CMemMap;
private:
	CameraState		m_cameraState;
	bool	m_buttonPressed; // For input
	CUtlVector< CameraEntity >	m_inventory;
	int		m_current_inventory_slot;

	float	m_next_scale_time;

	DECLARE_DATADESC();
};

#endif // WEAPON_CAMERA_H