#include "cbase.h"

#include "beam_shared.h"
#include "props.h"
#include "ndebugoverlay.h"
#include "IEffects.h"
#include "portal_player.h"
#include "soundenvelope.h"

#include "npc_turret_floor.h"

#include "prop_laser_emitter.h"
#include "prop_laser_catcher.h"

#define LASER_BEAM_SPRITE "sprites/purplelaser1.vmt"//"sprites/xbeam2.vmt"
#define LASER_EMITTER_DEFAULT_SPRITE "sprites/purpleglow1.vmt"

#define LASER_ACTIVATION_SOUND "Laser.Activate"
#define LASER_AMBIENCE_SOUND "vfx/laser_beam_lp_01.wav"

#define LASER_PARTICLE "laser_fx"

#define LASER_AMBIENCE_SOUND_VOLUME 0.1f

ConVar portal_laser_colour("portal_laser_colour", "255 64 64", FCVAR_REPLICATED, "Set the colour of the laser beams. Note: You need to reload the map to apply changes!");
ConVar portal_laser_texture("portal_laser_texture", LASER_BEAM_SPRITE, FCVAR_REPLICATED, "Set the texture of the laser beams. Note: You need to reload the map to apply changes!");
ConVar portal_laser_effect("portal_laser_effect", LASER_PARTICLE, FCVAR_REPLICATED, "Effect plays on the emitter when it's active.");
ConVar portal_laser_sfx_volume("portal_laser_sfx_volume", "-1", FCVAR_REPLICATED, "Laser's buzzing sound volume. Use -1 for default volume.");
ConVar portal_laser_debug("portal_laser_debug", "0", FCVAR_CHEAT, "Show laser debug informations.");
ConVar portal_laser_pushforce("portal_laser_pushforce", "200", FCVAR_CHEAT, "Set the push force of the laser.");
ConVar portal_laser_pushradius("portal_laser_pushradius", "4", FCVAR_CHEAT, "Radius of the pusher box.");

LINK_ENTITY_TO_CLASS(env_portal_laser, CEnvPortalLaser)

BEGIN_DATADESC(CEnvPortalLaser)
// Fields
	DEFINE_SOUNDPATCH(m_pLaserSound),
// Key fields
	DEFINE_KEYFIELD(m_bStartActive, FIELD_BOOLEAN, "startactive"),
// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "TurnOn", InputTurnOn),
	DEFINE_INPUTFUNC(FIELD_VOID, "TurnOff", InputTurnOff),
	DEFINE_INPUTFUNC(FIELD_VOID, "Toggle", InputToggle),
// Think functions
	DEFINE_THINKFUNC(LaserThink),
END_DATADESC()

CEnvPortalLaser::~CEnvPortalLaser() {
	if (m_pBeam) {
		UTIL_Remove(m_pBeam);
	}

	DestroySounds();
}

CEnvPortalLaser::CEnvPortalLaser() : m_pBeam(NULL), m_pCatcher(NULL), BaseClass() { }

void CEnvPortalLaser::Precache() {
	PrecacheScriptSound(LASER_ACTIVATION_SOUND);
	PrecacheSound(LASER_AMBIENCE_SOUND);
	PrecacheParticleSystem(LASER_PARTICLE);

	m_iSprite = PrecacheModel(LASER_EMITTER_DEFAULT_SPRITE);

	BaseClass::Precache();
}

void CEnvPortalLaser::Spawn() {
	BaseClass::Spawn();

	if (m_bStartActive) {
		TurnOn();
	} else {
		TurnOff();
	}

	SetSolid(SOLID_VPHYSICS);

	CreateVPhysics();
}

void CEnvPortalLaser::LaserThink() {
	Vector vecStart = GetAbsOrigin(), vecEnd;
	Vector vecPortalIn, vecPortalOut;
	Vector vecDir, traceDir;
	QAngle angDir = GetAbsAngles();
	trace_t tr;
	AngleVectors(GetAbsAngles(), &vecDir);

	traceDir = vecDir;

	if (UTIL_Portal_Trace_Beam(m_pBeam, vecStart, vecEnd, vecPortalIn, vecPortalOut, MASK_BLOCKLOS, NULL)) {
		Vector vecOrigin = GetAbsOrigin();
		QAngle angDir = GetAbsAngles();

		m_pBeam->PointsInit(vecOrigin, vecOrigin + vecDir * MAX_TRACE_LENGTH);

		traceDir = (vecEnd - vecPortalOut).Normalized();
		UTIL_TraceLine(vecPortalOut, vecPortalOut + (traceDir * MAX_TRACE_LENGTH), MASK_BLOCKLOS, NULL, &tr);
		HandlePlayerKnockback(vecDir, GetAbsOrigin());
		HandlePlayerKnockback(traceDir, vecPortalOut);
	} else {
		Vector vecOrigin = GetAbsOrigin();
		QAngle angDir = GetAbsAngles();
		UTIL_TraceLine(vecOrigin, vecOrigin + (traceDir * MAX_TRACE_LENGTH), MASK_BLOCKLOS, NULL, &tr);
		HandlePlayerKnockback(vecDir, GetAbsOrigin());

		m_pBeam->PointsInit(vecOrigin, tr.endpos);
	}

	bool bSparks = true;

	if (tr.m_pEnt) {
		// Check if we hit a laser detector
		if (Q_strcmp(tr.m_pEnt->GetClassname(), "func_laser_detect") == 0) {
			if (portal_laser_debug.GetBool()) {
				NDebugOverlay::Cross3D(tr.endpos, 16, 0xFF, 0x00, 0x00, false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
			}

			CFuncLaserDetector* pCatcher = dynamic_cast<CFuncLaserDetector*>(tr.m_pEnt);
			// Check if casting to catcher successed
			if (pCatcher) {
				// Check if the catcher is different
				if (m_pCatcher != pCatcher) {
					if (m_pCatcher != NULL) {
						m_pCatcher->RemoveEmitter(this);
					}
					pCatcher->AddEmitter(this);
					m_pCatcher = pCatcher;
				}
			}
			bSparks = false;
		} else {
			// If we did not hit a laser detector, check if we did in the past and remove this emitter.
			if (m_pCatcher != NULL) {
				m_pCatcher->RemoveEmitter(this);
				m_pCatcher = NULL;
			}
			
			if (Q_strcmp(tr.m_pEnt->GetClassname(), "npc_portal_turret_floor") == 0) {
				if (portal_laser_debug.GetBool()) {
					NDebugOverlay::Cross3D(tr.endpos, 16, 0xFF, 0x00, 0x00, false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
				}
				CNPC_FloorTurret* pTurret = dynamic_cast<CNPC_FloorTurret*>(tr.m_pEnt);
				if (pTurret != NULL) {
					pTurret->SelfDestruct();
				}
			}
		}
	}

	if (bSparks) {
		g_pEffects->MetalSparks(tr.endpos, -traceDir);
	}
	SetNextThink(gpGlobals->curtime);
}

void CEnvPortalLaser::TurnOn() {
	if (!m_bStatus) {
		Vector vecDir, vecOrigin = GetAbsOrigin();
		QAngle angDir = GetAbsAngles();
		AngleVectors(angDir, &vecDir);

		CBaseAnimating* pAnim = dynamic_cast<CBaseAnimating*>(GetParent());
		if (pAnim != NULL && !pAnim->GetAttachment("laser_attachment", vecOrigin, angDir)) {
			vecOrigin = GetAbsOrigin();
			angDir = GetAbsAngles();
		}

		if (m_pBeam == NULL) {
			m_pBeam = (CBeam*)CreateEntityByName("beam");
			if (m_pBeam) {
				m_pBeam->SetBeamTraceMask(MASK_BLOCKLOS);
				const char* szSprite = portal_laser_texture.GetString();
				if (szSprite == NULL || Q_strlen(szSprite) == 0) {
					szSprite = LASER_BEAM_SPRITE;
				}
				m_pBeam->BeamInit(szSprite, 2);

				int r = 0xFF, g = 0x00, b = 0x00;
				const char* szColours = portal_laser_colour.GetString();
				if (szColours != NULL && Q_strlen(szColours) > 0) {
					sscanf(szColours, "%i%i%i", &r, &g, &b);
				}

				m_pBeam->SetColor(r, g, b);
				m_pBeam->SetBrightness(255);
				m_pBeam->SetCollisionGroup(COLLISION_GROUP_DEBRIS);
				m_pBeam->PointsInit(vecOrigin, vecOrigin + vecDir * MAX_TRACE_LENGTH);
				m_pBeam->SetStartEntity(this);
				m_pBeam->SetHaloTexture(m_iSprite);
				m_pBeam->SetHaloScale(10);
				// m_pBeam->SetParent(this);
			} else {
				Warning("Failed to create beam!");
			}
		} else {
			m_pBeam->RemoveEffects(EF_NODRAW);
		}

		const char* effect = portal_laser_effect.GetString();
		if (effect == NULL) {
			effect = LASER_PARTICLE;
		}

		m_bStatus = true;

		EmitSound(LASER_ACTIVATION_SOUND);
		CreateSounds();

		SetThink(&CEnvPortalLaser::LaserThink);
		SetNextThink(gpGlobals->curtime);
	}
}

void CEnvPortalLaser::TurnOff() {
	if (m_bStatus) {
		if (m_pBeam) {
			m_pBeam->AddEffects(EF_NODRAW);
		}

		if (m_pCatcher) {
			m_pCatcher->RemoveEmitter(this);
			m_pCatcher = NULL;
		}

		m_bStatus = false;
		DestroySounds();

		SetThink(NULL);
	}
}

void CEnvPortalLaser::Toggle() {
	if (m_bStatus) {
		TurnOff();
	} else {
		TurnOn();
	}
}

void CEnvPortalLaser::CreateSounds() {
	CSoundEnvelopeController& controller = CSoundEnvelopeController::GetController();

	CPASAttenuationFilter filter(this);
	if (!m_pLaserSound) {
		m_pLaserSound = controller.SoundCreate(filter, entindex(), LASER_AMBIENCE_SOUND);
	}

	float value = portal_laser_sfx_volume.GetFloat();
	controller.Play(m_pLaserSound, value > -1 ? value : LASER_AMBIENCE_SOUND_VOLUME, 100);
}

void CEnvPortalLaser::DestroySounds() {
	CSoundEnvelopeController& controller = CSoundEnvelopeController::GetController();

	controller.SoundDestroy(m_pLaserSound);
	m_pLaserSound = NULL;
}

void CEnvPortalLaser::InputTurnOn(inputdata_t& data) {
	TurnOn();
}

void CEnvPortalLaser::InputTurnOff(inputdata_t& data) {
	TurnOff();
}

void CEnvPortalLaser::InputToggle(inputdata_t& data) {
	Toggle();
}

void CEnvPortalLaser::HandlePlayerKnockback(const Vector& vecDir, const Vector& vecStart) {
	QAngle angDir;
	VectorAngles(vecDir, angDir);
	Vector vecForward, vecRight, vecUp;
	trace_t tr;
	AngleVectors(angDir, &vecForward, &vecRight, &vecUp);

	Vector vecOrigins[] = {
		vecStart + (vecRight * portal_laser_pushradius.GetFloat()) + (vecUp * portal_laser_pushradius.GetFloat()),
		vecStart + (vecRight * portal_laser_pushradius.GetFloat()) - (vecUp * portal_laser_pushradius.GetFloat()),
		vecStart - (vecRight * portal_laser_pushradius.GetFloat()) + (vecUp * portal_laser_pushradius.GetFloat()),
		vecStart - (vecRight * portal_laser_pushradius.GetFloat()) - (vecUp * portal_laser_pushradius.GetFloat())
	};
	Vector vecPushDir[] = {
		vecRight,
		vecRight,
		-vecRight,
		-vecRight,
	};

	for (int i = 0; i < 4; i++) {
		UTIL_TraceLine(vecOrigins[i], vecOrigins[i] + vecDir * MAX_TRACE_LENGTH, MASK_BLOCKLOS, NULL, &tr);
		if (tr.m_pEnt && tr.m_pEnt->IsPlayer()) {
			tr.m_pEnt->VelocityPunch(vecPushDir[i] * portal_laser_pushforce.GetFloat());

			tr.m_pEnt->TakeDamage(CTakeDamageInfo(this, this, 1, DMG_BURN));
			if (m_fHurnSoundTime < gpGlobals->curtime) {
				tr.m_pEnt->EmitSound("Player.BurnPain");
				m_fHurnSoundTime = gpGlobals->curtime + 0.5f;
			}
		}

		if (portal_laser_debug.GetBool()) {
			NDebugOverlay::Line(tr.startpos, tr.endpos, 0xFF, 0x00, 0x00, false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
		}
	}
}

// Laser emitter prop

LINK_ENTITY_TO_CLASS(prop_laser_emitter, CPropLaserEmitter);

BEGIN_DATADESC(CPropLaserEmitter)
// Fields
	DEFINE_FIELD(m_pLaser, FIELD_CLASSPTR),
// Key fields
	DEFINE_KEYFIELD(m_bStartActive, FIELD_BOOLEAN, "startactive"),
// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "TurnOn", InputTurnOn),
	DEFINE_INPUTFUNC(FIELD_VOID, "TurnOff", InputTurnOff),
	DEFINE_INPUTFUNC(FIELD_VOID, "Toggle", InputToggle),
END_DATADESC()

CPropLaserEmitter::CPropLaserEmitter() : m_pLaser(NULL) {
}

CPropLaserEmitter::~CPropLaserEmitter() {
	if (m_pLaser) {
		UTIL_Remove(m_pLaser);
	}
}

void CPropLaserEmitter::Precache() {
	string_t sModel = GetModelName();
	if (sModel != NULL_STRING) {
		PrecacheModel(STRING(sModel));
	}

	BaseClass::Precache();
}
void CPropLaserEmitter::Spawn() {
	m_pLaser = dynamic_cast<CEnvPortalLaser*>(CreateEntityByName("env_portal_laser"));
	if (m_pLaser != NULL) {
		m_pLaser->Precache();
		DispatchSpawn(m_pLaser);

		m_pLaser->SetParent(this);
		if (m_bStartActive) {
			m_pLaser->TurnOn();
		} else {
			m_pLaser->TurnOff();
		}
	}

	CreateVPhysics();
	SetSolid(SOLID_VPHYSICS);

	BaseClass::Spawn();
}

void CPropLaserEmitter::TurnOn() {
	if (m_pLaser != NULL) {
		m_pLaser->TurnOn();
	}
}
void CPropLaserEmitter::TurnOff() {
	if (m_pLaser != NULL) {
		m_pLaser->TurnOff();
	}
}
void CPropLaserEmitter::Toggle() {
	if (m_pLaser != NULL) {
		m_pLaser->Toggle();
	}
}

void CPropLaserEmitter::InputTurnOn(inputdata_t& data) {
	TurnOn();
}
void CPropLaserEmitter::InputTurnOff(inputdata_t& data) {
	TurnOff();
}
void CPropLaserEmitter::InputToggle(inputdata_t& data) {
	Toggle();
}
