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

#define LASER_EMITTER_MODEL "models/props/laser_emitter_center.mdl"
#define LASER_BEAM_SPRITE "sprites/xbeam2.vmt"

#define LASER_ACTIVATION_SOUND "Laser.Activate"
#define LASER_AMBIENCE_SOUND "vfx/laser_beam_lp_01.wav"

#define LASER_PARTICLE "laser_fx"

#define LASER_AMBIENCE_SOUND_VOLUME 0.1f

ConVar portal_laser_colour("portal_laser_colour", "255 64 64", FCVAR_REPLICATED, "Set the colour of the laser beams. Note: You need to reload the map to apply changes!");
ConVar portal_laser_texture("portal_laser_texture", LASER_BEAM_SPRITE, FCVAR_REPLICATED, "Set the texture of the laser beams. Note: You need to reload the map to apply changes!");
ConVar portal_laser_effect("portal_laser_effect", LASER_PARTICLE, FCVAR_REPLICATED, "Effect plays on the emitter when it's active.");
ConVar portal_laser_sfx_volume("portal_laser_sfx_volume", "-1", FCVAR_REPLICATED, "Laser's buzzing sound volume. Use -1 for default volume.");
ConVar portal_laser_debug("portal_laser_debug", "0", FCVAR_CHEAT, "Show laser debug informations.");

LINK_ENTITY_TO_CLASS(env_portal_laser, CPropLaserEmitter)
LINK_ENTITY_TO_CLASS(prop_laser_emitter, CPropLaserEmitter)

BEGIN_DATADESC(CPropLaserEmitter)
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

CPropLaserEmitter::~CPropLaserEmitter() {
	if (m_pBeam) {
		UTIL_Remove(m_pBeam);
	}
	if (m_pLaserFx) {
		UTIL_Remove(m_pLaserFx);
	}

	DestroySounds();
}

CPropLaserEmitter::CPropLaserEmitter() : m_pBeam(NULL), m_pLaserFx(NULL), m_pCatcher(NULL), BaseClass() { }

void CPropLaserEmitter::Precache() {
	PrecacheScriptSound(LASER_ACTIVATION_SOUND);
	PrecacheSound(LASER_AMBIENCE_SOUND);
	PrecacheParticleSystem(LASER_PARTICLE);

	BaseClass::Precache();
}

void CPropLaserEmitter::Spawn() {
	BaseClass::Spawn();

	if (m_bStartActive) {
		TurnOn();
	} else {
		TurnOff();
	}

	SetSolid(SOLID_VPHYSICS);

	CreateVPhysics();
}

void CPropLaserEmitter::LaserThink() {
	Vector vecStart, vecEnd;
	Vector vecPortalIn, vecPortalOut;
	Vector vecDir, traceDir;
	trace_t tr;
	AngleVectors(GetAbsAngles(), &vecDir);

	traceDir = vecDir;

	if (UTIL_Portal_Trace_Beam(m_pBeam, vecStart, vecEnd, vecPortalIn, vecPortalOut, NULL)) {
		Vector vecOrigin = GetAbsOrigin();
		QAngle angDir;
		// Get laser attachment, if it doesn't exists, get the model's abs origin and abs angles.
		int iAttach = LookupAttachment("laser_attachment");
		if (iAttach > -1) {
			GetAttachment(iAttach, vecOrigin, angDir);
		}

		m_pBeam->PointsInit(vecOrigin, vecOrigin + vecDir * MAX_TRACE_LENGTH);

		traceDir = (vecEnd - vecPortalOut).Normalized();
		UTIL_TraceLine(vecPortalOut, vecPortalOut + (traceDir * MAX_TRACE_LENGTH), MASK_BLOCKLOS, NULL, &tr);
	}
	else {
		Vector vecOrigin = GetAbsOrigin();
		QAngle angDir;
		// Get laser attachment, if it doesn't exists, get the model's abs origin and abs angles.
		int iAttach = LookupAttachment("laser_attachment");
		if (iAttach > -1) {
			GetAttachment(iAttach, vecOrigin, angDir);
		}
		UTIL_TraceLine(vecOrigin, vecOrigin + (traceDir * MAX_TRACE_LENGTH), MASK_BLOCKLOS, NULL, &tr);

		m_pBeam->PointsInit(vecOrigin, tr.endpos);
	}

	bool bSparks = true;

	if (tr.m_pEnt) {
		// Check if we hit a laser detector
		if (Q_strcmp(tr.m_pEnt->GetClassname(), "func_laser_detect") == 0) {
			if (portal_laser_debug.GetBool()) {
				NDebugOverlay::Cross3D(tr.endpos, 16, 0xFF, 0x00, 0x00, false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
			}
			// Check if no other catcher is active
			if (m_pCatcher == NULL) {
				CFuncLaserDetector* pCatcher = dynamic_cast<CFuncLaserDetector*>(tr.m_pEnt);
				pCatcher->AddEmitter(this);
				m_pCatcher = pCatcher;
			}

			bSparks = false;
		} else {
			// If we did not hit a laser detector, check if we did in the past and remove this emitter.
			if (m_pCatcher != NULL) {
				m_pCatcher->RemoveEmitter(this);
				m_pCatcher = NULL;
			}

			// Push player away from the beam
			if (tr.m_pEnt->IsPlayer()) {
				if (portal_laser_debug.GetBool()) {
					NDebugOverlay::Cross3D(tr.endpos, 16, 0xFF, 0x00, 0x00, false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
				}				CPortal_Player* player = ToPortalPlayer(tr.m_pEnt);
				if (player) {
					Vector pushAwayForce = (player->GetAbsOrigin() - tr.endpos).Normalized() * 100;

					player->TakeDamage(CTakeDamageInfo(this, this, Vector(), tr.endpos, 1.0f, DMG_BURN));
					player->ApplyLocalVelocityImpulse(pushAwayForce);

					if (m_fPainTimer < gpGlobals->curtime) {
						player->EmitSound("Player.BurnPain");
						m_fPainTimer = gpGlobals->curtime + 0.5f;
					}
				}
			// Let's check if we hit a turret
			} else if (Q_strcmp(tr.m_pEnt->GetClassname(), "npc_portal_turret_floor") == 0) {
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

void CPropLaserEmitter::TurnOn() {
	if (!m_bStatus) {
		Vector vecDir, vecOrigin = GetAbsOrigin();
		QAngle angDir = GetAbsAngles();

		// Get laser attachment, if it doesn't exists, get the model's abs origin and abs angles.
		int iAttach = LookupAttachment("laser_attachment");
		if (iAttach > -1) {
			GetAttachment(iAttach, vecOrigin, angDir);
		}
		AngleVectors(angDir, &vecDir);

		if (m_pBeam == NULL) {
			m_pBeam = (CBeam*)CreateEntityByName("beam");
			if (m_pBeam) {
				const char* spr = portal_laser_texture.GetString();
				if (spr == NULL) {
					spr = LASER_BEAM_SPRITE;
				}
				m_pBeam->BeamInit(spr, 1);

				int r = 0xFF, g = 0x00, b = 0x00;
				const char* colours = portal_laser_colour.GetString();
				if (colours != NULL) {
					sscanf(colours, "%i%i%i", &r, &g, &b);
				}

				m_pBeam->SetColor(r, g, b);
				m_pBeam->SetBrightness(255);
				m_pBeam->SetCollisionGroup(COLLISION_GROUP_DEBRIS);
				m_pBeam->PointsInit(vecOrigin, vecOrigin + vecDir * MAX_TRACE_LENGTH);
				m_pBeam->SetStartEntity(this);
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
		if (m_pLaserFx == NULL) {
			m_pLaserFx = (CParticleSystem*)CreateEntityByName("info_particle_system");
			if (m_pLaserFx != NULL) {
				// Setup our basic parameters
				m_pLaserFx->KeyValue("start_active", "1");
				m_pLaserFx->KeyValue("effect_name", effect);
				m_pLaserFx->SetAbsOrigin(vecOrigin);
				m_pLaserFx->SetAbsAngles(GetAbsAngles());
				DispatchSpawn(m_pLaserFx);
				m_pLaserFx->Activate();
				m_pLaserFx->StartParticleSystem();
				m_pLaserFx->SetParent(this);
			} else {
				Warning("Failed to create laser effect!");
			}
		} else {
			m_pLaserFx->KeyValue("effect_name", effect);
			m_pLaserFx->Activate();
			m_pLaserFx->StartParticleSystem();
		}

		m_bStatus = true;

		EmitSound(LASER_ACTIVATION_SOUND);
		CreateSounds();

		SetThink(&CPropLaserEmitter::LaserThink);
		SetNextThink(gpGlobals->curtime);
	}
}

void CPropLaserEmitter::TurnOff() {
	if (m_bStatus) {
		if (m_pBeam) {
			m_pBeam->AddEffects(EF_NODRAW);
		}

		if (m_pLaserFx) {
			m_pLaserFx->StopParticleSystem();
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

void CPropLaserEmitter::Toggle() {
	if (m_bStatus) {
		TurnOff();
	} else {
		TurnOn();
	}
}

void CPropLaserEmitter::CreateSounds() {
	CSoundEnvelopeController& controller = CSoundEnvelopeController::GetController();

	CPASAttenuationFilter filter(this);
	if (!m_pLaserSound) {
		m_pLaserSound = controller.SoundCreate(filter, entindex(), LASER_AMBIENCE_SOUND);
	}

	float value = portal_laser_sfx_volume.GetFloat();
	controller.Play(m_pLaserSound, value > -1 ? value : LASER_AMBIENCE_SOUND_VOLUME, 100);
}

void CPropLaserEmitter::DestroySounds() {
	CSoundEnvelopeController& controller = CSoundEnvelopeController::GetController();

	controller.SoundDestroy(m_pLaserSound);
	m_pLaserSound = NULL;
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

CPropLaserEmitter* CPropLaserEmitter::Create(const Vector& origin, const QAngle& angle, const char* propModel, CBaseEntity* parent) {
	CPropLaserEmitter* pEnt = CreateNoModel(origin, angle, parent);
	if (pEnt != NULL) {
		pEnt->SetModel(propModel);
	}

	return pEnt;
}

CPropLaserEmitter* CPropLaserEmitter::CreateNoModel(const Vector& origin, const QAngle& angle, CBaseEntity* parent) {
	CPropLaserEmitter* pEnt = (CPropLaserEmitter*)CreateEntityByName("prop_laser_emitter");
	if (pEnt != NULL) {
		DispatchSpawn(pEnt);
		pEnt->SetAbsOrigin(origin);
		pEnt->SetAbsAngles(angle);
		pEnt->SetParent(parent);
	}

	return pEnt;
}
