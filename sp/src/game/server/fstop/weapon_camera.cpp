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
#include <props.h>


static const int	CAMERA_MAX_INVENTORY = 3;
static const int	CAMERA_MIN_SCALE = 0.25;
static const int	CAMERA_MAX_SCALE = 2;
static const float	CAMERA_SCALE_STEP = 0.5;
static const char*	CAMERA_BLACKLIST[] = {
	"worldspawn",
	"prop_vehicle"
};
static const int	CAMERA_BLACKLIST_LEN = 2;

BEGIN_DATADESC(CWeaponCamera)

	DEFINE_FIELD(m_buttonPressed, FIELD_BOOLEAN),
	DEFINE_FIELD(m_cameraState, FIELD_INTEGER),
	DEFINE_UTLVECTOR(m_inventory, FIELD_EMBEDDED),
	DEFINE_FIELD(m_next_scale_time, FIELD_FLOAT),

END_DATADESC()

BEGIN_SIMPLE_DATADESC(CameraEntity)
	DEFINE_FIELD(entity, FIELD_EHANDLE),
	DEFINE_FIELD(moveType, FIELD_INTEGER),
	DEFINE_FIELD(solidType, FIELD_INTEGER),
	DEFINE_FIELD(effects, FIELD_INTEGER),
	DEFINE_FIELD(shouldWake, FIELD_BOOLEAN),
	DEFINE_FIELD(modelRadius, FIELD_FLOAT),
	DEFINE_FIELD(modelHeight, FIELD_FLOAT),
END_DATADESC()


IMPLEMENT_SERVERCLASS_ST(CWeaponCamera, DT_WeaponCamera)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_camera, CWeaponCamera );
PRECACHE_WEAPON_REGISTER( weapon_camera );



//-----------------------------------------------------------------------------
// Purpose: Camera Entity Functions
//-----------------------------------------------------------------------------
void CameraEntity::CaptureEntity(void) {
	CBaseEntity* baseEntity = entity.Get();
	Vector modelSize = baseEntity->CollisionProp()->OBBMaxs() - baseEntity->CollisionProp()->OBBMins();
	modelRadius = baseEntity->CollisionProp()->BoundingRadius();

	moveType = baseEntity->GetMoveType();			// Original Movement Type
	solidType = baseEntity->GetSolid();				// Original solid type
	effects = baseEntity->GetEffects();				// Original effects
	shouldWake = baseEntity->VPhysicsGetObject()->IsGravityEnabled() || baseEntity->VPhysicsGetObject()->IsMoveable();	// Originally asleep

	HideEntity(); // Hides the entity
}

void CameraEntity::RestoreEntity(bool solidify) {
	CBaseEntity* baseEntity = entity.Get();

	// Update collider location
	baseEntity->VPhysicsGetObject()->SetPosition(baseEntity->GetAbsOrigin(), baseEntity->GetAbsAngles(), true);
	baseEntity->VPhysicsUpdate(baseEntity->VPhysicsGetObject());
	baseEntity->VPhysicsShadowUpdate(baseEntity->VPhysicsGetObject());

	if (solidify) {
		// Restore data
		baseEntity->SetSolid(solidType);
		baseEntity->SetMoveType(moveType);

		if (shouldWake) {
			baseEntity->VPhysicsGetObject()->Wake(); // Enable 
		}
	}

	baseEntity->SetEffects(effects);
}

void CameraEntity::HideEntity(void) {
	CBaseEntity* baseEntity = entity.Get();
	// Disable collision
	baseEntity->SetMoveType(MOVETYPE_NONE); // No physics movement at all
	baseEntity->SetSolid(SOLID_NONE); // Wouldn't want it to hit anything
	baseEntity->AddEffects(EF_NODRAW); // NODRAW for performance purposes

	// Teleport to OOB (with any luck)
	baseEntity->SetAbsOrigin(Vector(3000, 3000, 3000));

	// Update collider location
	baseEntity->VPhysicsGetObject()->SetPosition(baseEntity->GetAbsOrigin(), baseEntity->GetAbsAngles(), true);
	baseEntity->VPhysicsUpdate(baseEntity->VPhysicsGetObject());
}



// Command to switch camera slot
void SwitchCameraSlot_f( const CCommand& args ) {
	if (args.ArgC() < 1 || *args.Arg(1) == *"")
	{
		Msg("Usage: camera_slot <slot#>\n");
		return;
	}

	CBasePlayer* pOwner = ToBasePlayer(UTIL_GetCommandClient());
	CWeaponCamera* cameraWeapon = dynamic_cast<CWeaponCamera*>(pOwner->Weapon_OwnsThisType("weapon_camera"));
	if (!cameraWeapon) {
		Msg("Player does not have camera!\n");
		return;
	}

	cameraWeapon->SetSlot(atoi(args.Arg(1)));
}
ConCommand switchCameraSlot("camera_slot", SwitchCameraSlot_f, "camera_slot <slot#>");

// Command to scale object
void ChangeCameraScale_f(const CCommand& args) {
	if (args.ArgC() < 1 || *args.Arg(1) == *"")
	{
		Msg("Usage: camera_slot <slot#>\n");
		return;
	}

	CBasePlayer* pOwner = ToBasePlayer(UTIL_GetCommandClient());
	CWeaponCamera* cameraWeapon = dynamic_cast<CWeaponCamera*>(pOwner->Weapon_OwnsThisType("weapon_camera"));
	if (!cameraWeapon) {
		Msg("Player does not have camera!\n");
		return;
	}

	cameraWeapon->ChangeScale(atoi(args.Arg(1)) == 1 ? true : false);
}
ConCommand changeCameraScale("camera_scale", ChangeCameraScale_f, "camera_scale <scale type>");



//-----------------------------------------------------------------------------
// Purpose: Precache
//-----------------------------------------------------------------------------
void CWeaponCamera::Precache( void )
{
	BaseClass::Precache();

	PrecacheScriptSound( "NPC_CScanner.TakePhoto" );
	PrecacheScriptSound( "Metal_Box.ImpactSoft" );
	PrecacheScriptSound( "NPC_Alyx.Climb_Pipe_strain_1" );
	PrecacheScriptSound( "NPC_Alyx.Climb_Pipe_strain_2" );
	PrecacheScriptSound( "Cardboard.ImpactSoft" );
	PrecacheScriptSound( "SolidMetal.StepRight" );

	//PrecacheScriptSound("Weapon_FlareGun.Burn");
	//PrecacheScriptSound("Weapon_FlareGun.Single");
	//PrecacheScriptSound("Weapon_FlareGun.Reload");

	//UTIL_PrecacheOther( "env_flare" );
}


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponCamera::CWeaponCamera(void)
{
	m_flNextSecondaryAttack = gpGlobals->curtime;
	m_next_scale_time = gpGlobals->curtime;
	m_current_inventory_slot = 0;
	m_buttonPressed = false;
	SetThink(NULL); // No think, just do
}


//-----------------------------------------------------------------------------
// Purpose: Get the state of the camera
//-----------------------------------------------------------------------------
int CWeaponCamera::GetState(void)
{
	return m_cameraState;
}


//-----------------------------------------------------------------------------
// Purpose: Switch to slot and then enter placement mode
//-----------------------------------------------------------------------------
void CWeaponCamera::SetSlot(int slot)
{
	if (slot > m_inventory.Count()-1) { // If slot is not in the inventory
		Msg("Cannot switch to slot, slot invalid");
		return;
	}

	// If in placement mode, exitplacement mode and hide the entity
	if (m_cameraState == CAMERA_PLACEMENT) {
		SetThink(NULL);
		m_inventory[m_current_inventory_slot].HideEntity(); // Hide current placements
		m_cameraState = CAMERA_NORMAL;
	}

	// Set slot
	m_current_inventory_slot = slot;

	Msg("Slot Changed");
	SecondaryAttack(); // Enter placement mode
}


//-----------------------------------------------------------------------------
// Purpose: Set zoom
//-----------------------------------------------------------------------------
void CWeaponCamera::SetZoom(bool zoomState)
{
	CBasePlayer* pOwner = ToBasePlayer(GetOwner()); // Get player
	if (!pOwner)
		return;

	Msg("Setting zoom");
	if (zoomState) {
		pOwner->SetFOV(this, 30, 0.1f);
	}
	else {
		pOwner->SetFOV(this, 0, 0.2f);
	}

	// Send a message to hide the scope
	CSingleUserRecipientFilter filter(pOwner);
	UserMessageBegin(filter, "ShowCameraViewfinder");
	WRITE_BYTE(zoomState ? 1 : 0);
	MessageEnd();
}


//-----------------------------------------------------------------------------
// Purpose: Handle keyboard input from player
//-----------------------------------------------------------------------------
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
// Purpose: Handle placement logic as "think"
//-----------------------------------------------------------------------------
void CWeaponCamera::PlacementThink(void) {
	CBasePlayer* pOwner = ToBasePlayer(GetOwner()); // Get player
	if (!pOwner)
		return;

	// Traceline
	Vector dir;
	AngleVectors(pOwner->EyeAngles(), &dir);

	trace_t tr;
	UTIL_TraceLine(pOwner->EyePosition(), pOwner->EyePosition() + (dir * MAX_TRACE_LENGTH), MASK_SOLID, pOwner, COLLISION_GROUP_NONE, &tr);

	// Make item visible again
	CBaseAnimating* baseEntity = dynamic_cast<CBaseAnimating*>(m_inventory[m_current_inventory_slot].GetEntity());
	baseEntity->UpdateModelScale();

	baseEntity->SetAbsOrigin(tr.endpos - (dir * (m_inventory[m_current_inventory_slot].GetModelRadius())));
	QAngle angles = baseEntity->GetAbsAngles();
	angles.y += 0.5;
	baseEntity->SetAbsAngles(angles);

	m_inventory[m_current_inventory_slot].RestoreEntity(false); // Visible but not solid

	SetNextThink(gpGlobals->curtime + 0.01f);
}


//-----------------------------------------------------------------------------
// Purpose: Main attack (capture/place)
//-----------------------------------------------------------------------------
void CWeaponCamera::PrimaryAttack( void )
{
	m_flNextPrimaryAttack = gpGlobals->curtime;

	CBasePlayer* pOwner = ToBasePlayer(GetOwner()); // Get player
	if (!pOwner)
		return;

	Vector dir;
	trace_t tr;

	CPASAttenuationFilter filter(this);

	switch (m_cameraState) {
	case CAMERA_NORMAL:
		m_cameraState = CAMERA_ZOOM; // Switch to zoom mode
		SetZoom(true);

		//Play zoom sound
		EmitSound(filter, entindex(), "Cardboard.ImpactSoft");
		break;

	case CAMERA_ZOOM:
		SetZoom(false);
		m_cameraState = CAMERA_NORMAL; // Reset camera state

		// Check if can capture object
		if (m_inventory.Count() == CAMERA_MAX_INVENTORY) {
			Msg("INVENTORY IS FULL");
			return; // Inventory is full
		}

		// Traceline
		
		AngleVectors(pOwner->EyeAngles(), &dir);
		UTIL_TraceLine(pOwner->EyePosition(), pOwner->EyePosition() + (dir * MAX_TRACE_LENGTH), MASK_SOLID, pOwner, COLLISION_GROUP_NONE, &tr);


		if (tr.DidHit()) {
			CBaseEntity* baseEntity = dynamic_cast<CBaseEntity*>(tr.m_pEnt);

			// Check blacklist against entity
			for (int i = 0; i < CAMERA_BLACKLIST_LEN; i++) {
				if (strcmp(baseEntity->GetClassname(), CAMERA_BLACKLIST[i]) == 0) {
					Msg("Cannot capture entity");
					return;
				}
			}

			// Get Entity Info
			CameraEntity entityData;
			entityData.SetEntity(baseEntity);
			entityData.CaptureEntity(); // Do capture logic

			// Add entity to inventory
			m_inventory.AddToTail(entityData);

			//Play capture sound
			EmitSound(filter, entindex(), "NPC_CScanner.TakePhoto");
		}
		break;
	
	case CAMERA_PLACEMENT:
		SetThink(NULL); // Placement succeeds
		PlacementThink();
		m_inventory[m_current_inventory_slot].RestoreEntity();
		m_cameraState = CAMERA_NORMAL;
		m_inventory.Remove(m_current_inventory_slot); // Remove item from inventory
		if (m_current_inventory_slot > 0) { // Switch inventory slot
			m_current_inventory_slot -= 1;
		}

		//Play placement sound
		EmitSound(filter, entindex(), "Metal_Box.ImpactSoft");
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Secondary Attack (Switch To Placement Mode)
//-----------------------------------------------------------------------------
void CWeaponCamera::SecondaryAttack( void )
{
	Msg("Secondary Attack Triggered");
	m_flNextSecondaryAttack = gpGlobals->curtime;

	if (m_cameraState == CAMERA_ZOOM) {
		SetZoom(false);
	}

	if (m_cameraState == CAMERA_PLACEMENT) { // Exit out of placement mode
		SetThink(NULL);
		m_inventory[m_current_inventory_slot].HideEntity(); // Hide it again
		m_cameraState = CAMERA_NORMAL;
		return;
	}

	if (m_current_inventory_slot > m_inventory.Count()-1) { // Cannot switch to placement mode
		Msg("CANNOT SWITCH TO PLACE: Current inventory slot invalid, switchin slot");
		if (m_inventory.Count() > 0) {
			m_current_inventory_slot = m_inventory.Count() - 1;
		}
		else {
			Msg("Inventory is also empty");
			return;
		}
	}

	CPASAttenuationFilter filter(this);
	EmitSound(filter, entindex(), "SolidMetal.StepRight");

	m_cameraState = CAMERA_PLACEMENT; // Set placement state
	SetThink(&CWeaponCamera::PlacementThink);
	SetNextThink(gpGlobals->curtime);
}


//-----------------------------------------------------------------------------
// Purpose: Change placement scale
//-----------------------------------------------------------------------------
void CWeaponCamera::ChangeScale(bool scaleType)
{
	if (m_next_scale_time > gpGlobals->curtime) {
		return; // Not time for scale yet
	}
	if (m_cameraState != CAMERA_PLACEMENT) {
		return; // Cannot scale outside of placement
	}

	CPASAttenuationFilter filter(this);

	Msg("Scaling item");

	CBaseAnimating* baseEntity = dynamic_cast<CBaseAnimating*>(m_inventory[m_current_inventory_slot].GetEntity());

	if (baseEntity->GetModelScale() == (scaleType ? CAMERA_MAX_SCALE : CAMERA_MIN_SCALE)) { // Check if already at max/min scale for that "scale direction"
		Msg("Cannot scale further");
		return;
	}

	if (scaleType) {
		//Play scale up sound
		EmitSound(filter, entindex(), "NPC_Alyx.Climb_Pipe_strain_1");
	}
	else {
		//Play scale down sound
		EmitSound(filter, entindex(), "NPC_Alyx.Climb_Pipe_strain_2");
	}

	float targetScale = baseEntity->GetModelScale() + (scaleType ? CAMERA_SCALE_STEP : -CAMERA_SCALE_STEP);

	UTIL_CreateScaledPhysObject(baseEntity, targetScale, 0.25);
	CCollisionProperty* entityCollision = baseEntity->CollisionProp();
	entityCollision->RefreshScaledCollisionBounds();

	//Vector modelSize = baseEntity->CollisionProp()->OBBMaxs() - baseEntity->CollisionProp()->OBBMins();
	//m_inventory[m_current_inventory_slot].SetModelRadius(sqrt(pow(modelSize.x, 2) + pow(modelSize.y, 2)));
	//m_inventory[m_current_inventory_slot].SetModelHeight(modelSize.z);
	
	//entityCollision->SetCollisionBounds()

	m_next_scale_time = gpGlobals->curtime + 0.25;
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponCamera::Reload(void)
{
	return true;
}

