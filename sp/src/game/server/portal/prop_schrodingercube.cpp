//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Defines a Schrodinger storage cube.
//
//=====================================================================================//
#include "cbase.h"					// for pch
#include "prop_Schrodingercube.h"
#include "prop_laser_emitter.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	REFLECTION_MODEL	"models/props/reflection_cube.mdl"

#define FIZZLE_SOUND		"Prop.Fizzled"

#define LASER_EMITTER_DEFAULT_SPRITE "sprites/light_glow02_add.vmt"//"sprites/purpleglow1.vmt"

// CVar for visuals
// TODO: Finialize visuals and use macros/constants instead!
extern ConVar portal_laser_glow_sprite;
extern ConVar portal_laser_glow_sprite_scale;

enum SkinOld
{
	OLDReflective = 0
};

enum SkinType
{
	Clean = 0,
	Rusted = 1,
};

enum PaintPower
{
	Bounce = 0,
	Stick = 1,
	Speed,
	Portal,
	None,
};


LINK_ENTITY_TO_CLASS(prop_schrodinger_cube, CPropSchrodingerCube);


BEGIN_DATADESC(CPropSchrodingerCube)


// Save/load
DEFINE_USEFUNC(Use),

DEFINE_FIELD(m_pLaser, FIELD_CLASSPTR),
DEFINE_FIELD(m_pLaserSprite, FIELD_CLASSPTR),
DEFINE_FIELD(m_bSendingLaser, FIELD_BOOLEAN),

//DEFINE_KEYFIELD(m_oldSkin, FIELD_INTEGER, "skin"),
//DEFINE_KEYFIELD(m_cubeType, FIELD_INTEGER, "CubeType"),
//DEFINE_KEYFIELD(m_skinType, FIELD_INTEGER, "SkinType"),
DEFINE_KEYFIELD(m_linkageID, FIELD_INTEGER, "linkageid"),
DEFINE_KEYFIELD(m_paintPower, FIELD_INTEGER, "PaintPower"),
DEFINE_KEYFIELD(m_useNewSkins, FIELD_BOOLEAN, "NewSkins"),
DEFINE_KEYFIELD(m_allowFunnel, FIELD_BOOLEAN, "allowfunnel"),

DEFINE_INPUTFUNC(FIELD_VOID, "Dissolve", InputDissolve),
DEFINE_INPUTFUNC(FIELD_VOID, "SilentDissolve", InputSilentDissolve),
DEFINE_INPUTFUNC(FIELD_VOID, "PreDissolveJoke", InputPreDissolveJoke),
DEFINE_INPUTFUNC(FIELD_VOID, "ExitDisabledState", InputExitDisabledState),

// Output
//DEFINE_OUTPUT(m_OnPainted, "OnPainted"),

DEFINE_OUTPUT(m_OnFizzled, "OnFizzled"),
DEFINE_OUTPUT(m_OnOrangePickup, "OnOrangePickup"),
DEFINE_OUTPUT(m_OnBluePickup, "OnBluePickup"),
DEFINE_OUTPUT(m_OnPlayerPickup, "OnPlayerPickup"),
DEFINE_OUTPUT(m_OnPhysGunDrop, "OnPhysGunDrop"),

DEFINE_THINKFUNC(Think),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: Precache
// Input  :  - 
//-----------------------------------------------------------------------------
void CPropSchrodingerCube::Precache()
{
	PrecacheModel(REFLECTION_MODEL);
	PrecacheScriptSound(FIZZLE_SOUND);

	BaseClass::Precache();

}

int CPropSchrodingerCube::ObjectCaps()
{
	int caps = BaseClass::ObjectCaps();

	caps |= FCAP_IMPULSE_USE;

	return caps;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CPropSchrodingerCube::Spawn()
{
	Precache();


	SetModel(REFLECTION_MODEL);

	SetSolid(SOLID_VPHYSICS);

	// In order to pick it up, needs to be physics.
	CreateVPhysics();

	SetUse(&CPropSchrodingerCube::Use);

	// Create laser
	CEnvPortalLaser* pLaser = dynamic_cast<CEnvPortalLaser*>(CreateEntityByName("env_portal_laser"));
	pLaser->Activate(); // Should be called when map loads so I put it at the top

	Vector vOrigin;
	QAngle aAngle;
	this->GetAttachment("focus", vOrigin, aAngle);
	pLaser->SetAbsOrigin(vOrigin);
	pLaser->SetAbsAngles(aAngle);
	pLaser->SetParent(this, 0);

	// For Portal2-SDK13 Asset
	pLaser->SetLocalAngles(QAngle(0, 0, 90)); // Laser rotation is off, this may not be the case in p2009's model, check before merging
	pLaser->SetLocalOrigin(Vector(25, 0, 0)); // Offset the laser forwards from the bone by 25 units so it doesn't collide with the cube

	pLaser->TurnOff();
	DispatchSpawn(pLaser);
	m_pLaser = pLaser;


	// Create glow
	m_pLaserSprite = dynamic_cast<CSprite*>(CreateEntityByName("env_sprite"));
	if (m_pLaserSprite != NULL) {
		const char* szSprite = portal_laser_glow_sprite.GetString();
		if (szSprite == NULL || Q_strlen(szSprite) == 0) {
			szSprite = LASER_EMITTER_DEFAULT_SPRITE;
		}

		m_pLaserSprite->KeyValue("model", szSprite);
		m_pLaserSprite->SetGlowProxySize(this->CollisionProp()->BoundingRadius()); // Set glow radius accordingly
		m_pLaserSprite->Precache();
		m_pLaserSprite->SetAbsOrigin(this->GetAbsOrigin());
		m_pLaserSprite->SetAbsAngles(this->GetAbsAngles());
		m_pLaserSprite->SetParent(this, 0);
		DispatchSpawn(m_pLaserSprite);
		m_pLaserSprite->SetRenderMode(kRenderWorldGlow);
		m_pLaserSprite->SetScale(portal_laser_glow_sprite_scale.GetFloat());

		m_pLaserSprite->TurnOff();
	}

	BaseClass::Spawn();

}

CBaseEntity* CPropSchrodingerCube::GetLaser() {
	return m_pLaser;
}

bool CPropSchrodingerCube::PartnerRecieveLaser(bool state, byte beamR, byte beamG, byte beamB, byte spriteR, byte spriteG, byte spriteB)
{
	CEnvPortalLaser* pLaser = dynamic_cast<CEnvPortalLaser*>(m_pLaser);

	if (pLaser->GetState() == state) {
		return false; // If the cube is already "on/off" from from another laser, do nothing
	}

	pLaser->SetBeamColour(beamR, beamG, beamB);
	pLaser->SetSpriteColour(spriteR, spriteG, spriteB);

	m_pLaserSprite->SetRenderColor(spriteR, spriteG, spriteB);

	if (state == true) {
		pLaser->TurnOn();
		m_pLaserSprite->TurnOn();
	}
	else if (state == false) {
		pLaser->TurnOff();
		m_pLaserSprite->TurnOff();
	}

	return true;
}

bool CPropSchrodingerCube::PartnerSendLaser(bool state, byte beamR, byte beamG, byte beamB, byte spriteR, byte spriteG, byte spriteB)
{
	if (m_bSendingLaser == state) {
		return false; // If the cube is already sending/not-sending from from another laser, do nothing
	}
	m_bSendingLaser = state;

	// Find linkeage pair
	CBaseEntity* pEntity = NULL;
	while ((pEntity = gEntList.FindEntityByClassname(pEntity, "prop_schrodinger_cube")) != NULL) {
		if (pEntity == this) {
			continue;
		}

		CPropSchrodingerCube* cubeEnt = dynamic_cast<CPropSchrodingerCube*>(pEntity);

		if (cubeEnt->GetLinkageID() == m_linkageID) {
			CEnvPortalLaser* laser = dynamic_cast<CEnvPortalLaser*>(cubeEnt->GetLaser());
			if (laser->GetState() == state) {
				return false; // If the cube is already "on/off" from from another laser, do nothing
			}

			return cubeEnt->PartnerRecieveLaser(state, beamR, beamG, beamB, spriteR, spriteG, spriteB);
		}
	}

	return false;
}

void CPropSchrodingerCube::InputPreDissolveJoke(inputdata_t& data)
{
	// Sets some VO to do before fizzling.
}

void CPropSchrodingerCube::InputExitDisabledState(inputdata_t& data)
{
	// Exits the disabled state of a reflector cube.
}


void CPropSchrodingerCube::InputDissolve(inputdata_t& data)
{
	Dissolve(NULL, gpGlobals->curtime, false, 0, GetAbsOrigin(), 1);
	EmitSound(FIZZLE_SOUND);
}

void CPropSchrodingerCube::InputSilentDissolve(inputdata_t& data)
{
	Dissolve(NULL, gpGlobals->curtime, false, 0, GetAbsOrigin(), 1);
}

bool CPropSchrodingerCube::Dissolve(const char* materialName, float flStartTime, bool bNPCOnly, int nDissolveType, Vector vDissolverOrigin, int magnitude)
{
	m_OnFizzled.FireOutput(this, this);
	return BaseClass::Dissolve(materialName, flStartTime, bNPCOnly, nDissolveType, vDissolverOrigin, magnitude);
}

void CPropSchrodingerCube::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	CBasePlayer* pPlayer = ToBasePlayer(pActivator);
	if (pPlayer)
	{
		pPlayer->PickupObject(this);
	}
}

void CPropSchrodingerCube::OnPhysGunPickup(CBasePlayer* pPhysGunUser, PhysGunPickup_t reason)
{
	m_hPhysicsAttacker = pPhysGunUser;

	if (reason == PICKED_UP_BY_CANNON || reason == PICKED_UP_BY_PLAYER)
	{
		m_OnPlayerPickup.FireOutput(pPhysGunUser, this);
	}
}

void CPropSchrodingerCube::OnPhysGunDrop(CBasePlayer* pPhysGunUser, PhysGunDrop_t reason)
{
	m_OnPhysGunDrop.FireOutput(pPhysGunUser, this);
}

void CPropSchrodingerCube::SetActivated(bool active)
{
	if (m_skinType == Rusted)
	{
		if (active)
		{
			m_nSkin = 5;
		}
		else
		{
			m_nSkin = 1;
		}

	}
	else
	{
		if (active)
		{
			m_nSkin = 6;
		}
		else
		{
			m_nSkin = 0;
		}

	}

	if (m_paintPower == Bounce)
	{
		if (active)
		{
			m_nSkin = 7;
		}
		else
		{
			m_nSkin = 2;
		}
	}

	if (m_paintPower == Speed)
	{
		if (active)
		{
			m_nSkin = 8;
		}
		else
		{
			m_nSkin = 3;
		}
			
	}
}

