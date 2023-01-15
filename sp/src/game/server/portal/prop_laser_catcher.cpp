#include "cbase.h"
#include "prop_laser_catcher.h"
#include "soundenvelope.h"

#define CATCHER_ACTIVATE_SOUND "LaserCatcher.Activate"
#define CATCHER_DEACTIVATE_SOUND "LaserCatcher.Deactivate"
#define CATCHER_AMBIENCE_SOUND "world/laser_node_lp_01.wav"

LINK_ENTITY_TO_CLASS(func_laser_detect, CFuncLaserDetector)

BEGIN_DATADESC(CFuncLaserDetector)
// Key fields
	DEFINE_KEYFIELD(m_szIdleAnimation, FIELD_STRING, "idle_anim"),
	DEFINE_KEYFIELD(m_szActiveAnimation, FIELD_STRING, "active_anim"),
	DEFINE_KEYFIELD(m_bAnimateOnActive, FIELD_BOOLEAN, "animate_active"),
	DEFINE_KEYFIELD(m_szPropEntity, FIELD_STRING, "connected_prop"),
// Think functions
	DEFINE_THINKFUNC(DebugThink),
// Outputs
	DEFINE_OUTPUT(m_OnPowered, "OnPowered"),
	DEFINE_OUTPUT(m_OnUnpowered, "OnUnpowered"),
END_DATADESC()

extern ConVar portal_laser_debug;

void CFuncLaserDetector::Spawn() {
	SetSolid(SOLID_BBOX);

	if (m_szPropEntity != NULL) {
		m_pProp = gEntList.FindEntityByName(NULL, m_szPropEntity);
	}

	SetThink(&CFuncLaserDetector::DebugThink);
	SetNextThink(gpGlobals->curtime);
}

void CFuncLaserDetector::Precache() {
	PrecacheScriptSound(CATCHER_ACTIVATE_SOUND);
	PrecacheScriptSound(CATCHER_DEACTIVATE_SOUND);
	PrecacheSound(CATCHER_AMBIENCE_SOUND);

	BaseClass::Precache();
}

void CFuncLaserDetector::AddEmitter(CBaseEntity* emitter) {
	int oldCount = m_LaserList.size();

	if (m_LaserList.find(emitter) == m_LaserList.end()) {
		m_LaserList.insert(emitter);
	}

	if (oldCount == 0 && m_LaserList.size() > 0) {
		EmitSound(CATCHER_ACTIVATE_SOUND);
		m_OnPowered.FireOutput(NULL, NULL);
		if (m_pProp) {
			CBaseAnimating* pAnim = dynamic_cast<CBaseAnimating*>(m_pProp.Get());
			if (pAnim && m_szActiveAnimation != NULL) {
				int i = pAnim->LookupSequence(m_szActiveAnimation);
				if (i == -1) {
					pAnim->SetSequence(i);
				}
			}
			if (Q_strcmp(m_pProp->GetClassname(), "prop_laser_catcher") == 0) {
				CPropLaserCatcher* catcher = dynamic_cast<CPropLaserCatcher*>(m_pProp.Get());
				if (catcher) {
					catcher->FirePowerOnOutput();
				}
			}
		}

		CreateSounds();
	}
}

void CFuncLaserDetector::RemoveEmitter(CBaseEntity* emitter) {
	std::set<CBaseEntity*>::iterator it = m_LaserList.find(emitter);
	if (it != m_LaserList.end()) {
		m_LaserList.erase(it);
	}

	if (m_LaserList.size() == 0) {
		EmitSound(CATCHER_DEACTIVATE_SOUND);
		m_OnUnpowered.FireOutput(NULL, NULL);
		if (m_pProp) {
			CBaseAnimating* pAnim = dynamic_cast<CBaseAnimating*>(m_pProp.Get());
			if (pAnim && m_szIdleAnimation != NULL) {
				int i = pAnim->LookupSequence(m_szIdleAnimation);
				if (i == -1) {
					pAnim->SetSequence(i);
				}
			}
			if (Q_strcmp(m_pProp->GetClassname(), "prop_laser_catcher") == 0) {
				CPropLaserCatcher* catcher = dynamic_cast<CPropLaserCatcher*>(m_pProp.Get());
				if (catcher) {
					catcher->FirePowerOffOutput();
				}
			}
		}
		DestroySounds();
	}
}

void CFuncLaserDetector::CreateSounds() {
	CSoundEnvelopeController& controller = CSoundEnvelopeController::GetController();

	CPASAttenuationFilter filter(this);
	if (!m_pActiveSound) {
		m_pActiveSound = controller.SoundCreate(filter, entindex(), CATCHER_AMBIENCE_SOUND);
	}

	controller.Play(m_pActiveSound, 1, 100);
}

void CFuncLaserDetector::DestroySounds() {
	CSoundEnvelopeController& controller = CSoundEnvelopeController::GetController();

	controller.SoundDestroy(m_pActiveSound);
	m_pActiveSound = NULL;
}

bool CFuncLaserDetector::IsActivated() const {
	return m_bActivated;
}

CFuncLaserDetector* CFuncLaserDetector::Create(const Vector& origin, const QAngle& angles, const Vector& mins, const Vector& maxs, CBaseEntity* owner) {
	CFuncLaserDetector* pEnt = dynamic_cast<CFuncLaserDetector*>(CreateEntityByName("func_laser_detect"));
	if (pEnt == NULL) {
		return NULL;
	}

	UTIL_SetSize(pEnt, mins, maxs);
	pEnt->m_pProp = owner;
	pEnt->SetParent(owner);
	DispatchSpawn(pEnt);

	pEnt->SetAbsOrigin(origin);
	pEnt->SetAbsAngles(angles);

	return pEnt;
}

void CFuncLaserDetector::DebugThink() {
	if (portal_laser_debug.GetBool()) {
		DrawBBoxOverlay(1);

		NDebugOverlay::Axis(GetAbsOrigin(), GetAbsAngles(), 16, false, 1);
	}

	SetNextThink(gpGlobals->curtime + 1);
}

// ==== Catcher ====

LINK_ENTITY_TO_CLASS(prop_laser_catcher, CPropLaserCatcher);

BEGIN_DATADESC(CPropLaserCatcher)
	// Key fields
	DEFINE_KEYFIELD(m_szIdleAnimation, FIELD_STRING, "idle_anim"),
	DEFINE_KEYFIELD(m_szActiveAnimation, FIELD_STRING, "active_anim"),
	// Outputs
	DEFINE_OUTPUT(m_OnPowered, "OnPowered"),
	DEFINE_OUTPUT(m_OnUnpowered, "OnUnpowered"),
END_DATADESC()

void CPropLaserCatcher::Precache() {
	PrecacheScriptSound(CATCHER_ACTIVATE_SOUND);
	PrecacheScriptSound(CATCHER_DEACTIVATE_SOUND);
	PrecacheSound(CATCHER_AMBIENCE_SOUND);

	BaseClass::Precache();
}

CPropLaserCatcher::CPropLaserCatcher(): BaseClass() { }

void CPropLaserCatcher::Spawn() {
	BaseClass::Spawn();

	Vector vecOrigin;
	QAngle angDir;

	if (!GetAttachment("laser_target", vecOrigin, angDir)) {
		vecOrigin = GetAbsOrigin();
		angDir = GetAbsAngles();
	}

	m_bHoldAnimation = true;

	m_pLaserDetector = CFuncLaserDetector::Create(
		vecOrigin, angDir,
		Vector(-5, -10, -10), Vector(5, 10, 10),
		this
	);

	// Setup default animations if not specified
	if (m_szIdleAnimation == NULL) {
		m_szIdleAnimation = "idle";
	}
	if (m_szActiveAnimation == NULL) {
		m_szActiveAnimation = "spin";
	}

	SetSolid(SOLID_VPHYSICS);

	CreateVPhysics();
}

void CPropLaserCatcher::FirePowerOnOutput() {
	if (m_szActiveAnimation != NULL) {
		inputdata_t input;
		input.value.SetString(MAKE_STRING(m_szActiveAnimation));
		InputSetAnimation(input);
	}

	m_nSkin = 1;
	m_OnPowered.FireOutput(NULL, NULL);
}

void CPropLaserCatcher::FirePowerOffOutput() {
	if (m_szIdleAnimation != NULL) {
		inputdata_t input;
		input.value.SetString(MAKE_STRING(m_szIdleAnimation));
		InputSetAnimation(input);
	}

	m_nSkin = 0;
	m_OnUnpowered.FireOutput(NULL, NULL);
}