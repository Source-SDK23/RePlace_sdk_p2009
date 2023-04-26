//=========                                                        ============//
//
// Purpose:		Camera (FStop moment)
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "basehlcombatweapon.h"
#include "decals.h"
#include "soundenvelope.h"
#include "IEffects.h"
#include "engine/IEngineSound.h"
#include "weapon_camera.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC(CWeaponCamera)

	DEFINE_FIELD(m_buttonPressed, FIELD_BOOLEAN),
	DEFINE_FIELD(m_cameraState, FIELD_INTEGER)

END_DATADESC()


IMPLEMENT_SERVERCLASS_ST(CWeaponCamera, DT_WeaponCamera)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_camera, CWeaponCamera );
PRECACHE_WEAPON_REGISTER( weapon_camera );

//-----------------------------------------------------------------------------
// Purpose: Precache
//-----------------------------------------------------------------------------
void CWeaponCamera::Precache( void )
{
	BaseClass::Precache();

	//PrecacheScriptSound( "Flare.Touch" );

	//PrecacheScriptSound("Weapon_FlareGun.Burn");
	//PrecacheScriptSound("Weapon_FlareGun.Single");
	//PrecacheScriptSound("Weapon_FlareGun.Reload");

	//UTIL_PrecacheOther( "env_flare" );
}

CWeaponCamera::CWeaponCamera(void)
{
	m_flNextSecondaryAttack = gpGlobals->curtime;
	m_buttonPressed = false;
}

//====================================================================================
// WEAPON BEHAVIOUR
//====================================================================================
void CWeaponCamera::ItemPostFrame(void)
{
	CBasePlayer* pOwner = ToBasePlayer(GetOwner()); // Get player
	if (!pOwner)
		return;

	// If either attack button is released
	if (pOwner->m_afButtonReleased & IN_ATTACK || pOwner->m_afButtonReleased & IN_ATTACK2) {
		m_buttonPressed = false; // If button was just released
		return;
	}

	if (m_buttonPressed) {
		return; // Do not do anything as button is still being held down
	}

	UpdateAutoFire();

	//Track the duration of the fire
	//FIXME: Check for IN_ATTACK2 as well?
	//FIXME: What if we're calling ItemBusyFrame?
	m_fFireDuration = (pOwner->m_nButtons & IN_ATTACK) ? (m_fFireDuration + gpGlobals->frametime) : 0.0f;

	bool bFired = false;

	// Secondary attack has priority
	if ((pOwner->m_nButtons & IN_ATTACK2) && (m_flNextSecondaryAttack <= gpGlobals->curtime))
	{
		m_buttonPressed = true;
		SecondaryAttack();
	}

	if (!bFired && (pOwner->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		// If the firing button was just pressed, or the alt-fire just released, reset the firing time
		if ((pOwner->m_afButtonPressed & IN_ATTACK) || (pOwner->m_afButtonReleased & IN_ATTACK2))
		{
			m_flNextPrimaryAttack = gpGlobals->curtime;
		}

		m_buttonPressed = true;
		PrimaryAttack();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Main attack
//-----------------------------------------------------------------------------
void CWeaponCamera::PrimaryAttack( void )
{
	m_flNextPrimaryAttack = gpGlobals->curtime;

	CBasePlayer* pOwner = ToBasePlayer(GetOwner()); // Get player
	if (!pOwner)
		return;

	if (m_cameraState == CAMERA_NORMAL) {
		m_cameraState = CAMERA_ZOOM; // Switch to zoom mode
		
		// Actually zoom
	}
	else if (m_cameraState == CAMERA_ZOOM) {
		// Capture object
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCamera::SecondaryAttack( void )
{
	Msg("Secondary Attack Triggered");
	m_flNextSecondaryAttack = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponCamera::Reload(void)
{
	bool fRet = DefaultReload(GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD);
	if (fRet)
	{
		WeaponSound(RELOAD);
	}
	return fRet;
}

