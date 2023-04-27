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
#include <saverestore_utlvector.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Camera Entity Class
//BEGIN_DATADESC(CameraEntity)
//	DEFINE_FIELD(m_entity, FIELD_EHANDLE)
//END_DATADESC()


static const int	CAMERA_MAX_INVENTORY = 3;
static const int	CAMERA_MIN_SCALE = 0.25;
static const int	CAMERA_MAX_SCALE = 2;

BEGIN_DATADESC(CWeaponCamera)

	DEFINE_FIELD(m_buttonPressed, FIELD_BOOLEAN),
	DEFINE_FIELD(m_cameraState, FIELD_INTEGER),
	DEFINE_UTLVECTOR(m_inventory, FIELD_CLASSPTR),

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

	switch (m_cameraState) {
	case CAMERA_NORMAL:
		m_cameraState = CAMERA_ZOOM; // Switch to zoom mode
		break;

	case CAMERA_ZOOM:
		m_cameraState = CAMERA_NORMAL; // Reset camera state

		// Unzoom

		// Check if can capture object
		if (m_inventory.Size() == CAMERA_MAX_INVENTORY) {
			return; // Inventory is full
		}

		// Traceline
		Vector dir;
		AngleVectors(pOwner->EyeAngles(), &dir);

		trace_t tr;
		UTIL_TraceLine(pOwner->EyePosition(), pOwner->EyePosition() + (dir * MAX_TRACE_LENGTH), MASK_SOLID, pOwner, COLLISION_GROUP_NONE, &tr);


		if (tr.DidHit()) {
			Msg("RAY HIT!!!");
			Msg(tr.m_pEnt->GetClassname());
			Msg(tr.m_pEnt->GetEntityName().ToCStr());

			Vector modelSize = tr.m_pEnt->CollisionProp()->OBBMaxs() - tr.m_pEnt->CollisionProp()->OBBMins();
			double modelRadius = sqrt(pow(modelSize.x, 2) + pow(modelSize.y, 2));
			double modelHeight = modelSize.z;

			// Get Entity Info
			CameraEntity entityData = {
				tr.m_pEnt->GetRefEHandle(),			// Get entity handle
				tr.m_pEnt->GetMoveType(),			// Original Movement Type
				tr.m_pEnt->GetSolid(),				// Original solid type
				tr.m_pEnt->GetEffects(),			// Original effects
				modelRadius,						// Model Radius
				modelHeight							// Model Height
			};

			// Disable collision
			tr.m_pEnt->SetMoveType(MOVETYPE_NONE); // No physics movement at all
			tr.m_pEnt->SetSolid(SOLID_NONE); // Wouldn't want it to hit anything
			tr.m_pEnt->AddEffects(EF_NODRAW); // Performance purposes

			tr.m_pEnt->SetAbsOrigin(Vector(3000, 3000, 3000)); // Teleport away

			// Add entity to inventory
			m_inventory.AddToTail(&entityData);
		}

		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCamera::SecondaryAttack( void )
{
	CBasePlayer* pOwner = ToBasePlayer(GetOwner()); // Get player
	if (!pOwner)
		return;

	Msg("Secondary Attack Triggered");
	m_flNextSecondaryAttack = gpGlobals->curtime;

	if (m_cameraState == CAMERA_ZOOM) {
		// Unzoom
	}

	m_cameraState = CAMERA_PLACEMENT; // Set placement state

	// Traceline
	Vector dir;
	AngleVectors(pOwner->EyeAngles(), &dir);

	trace_t tr;
	UTIL_TraceLine(pOwner->EyePosition(), pOwner->EyePosition() + (dir * MAX_TRACE_LENGTH), MASK_ALL, pOwner, COLLISION_GROUP_NONE, &tr);

	Vector e = tr.startpos + ((tr.endpos - tr.startpos)*tr.fraction);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponCamera::Reload(void)
{
	return true;
}

