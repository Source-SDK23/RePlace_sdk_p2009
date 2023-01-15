#include "cbase.h"
#include "c_props.h"
#include "beam_shared.h"

#define LASER_BEAM_SPRITE "sprites/xbeam2.vmt"
#define LASER_PARTICLE "laser_fx"

ConVar portal_laser_colour("portal_laser_colour", "255 64 64", FCVAR_REPLICATED, "Set the colour of the laser beams. Note: You need to reload the map to apply changes!");
ConVar portal_laser_texture("portal_laser_texture", LASER_BEAM_SPRITE, FCVAR_REPLICATED, "Set the texture of the laser beams. Note: You need to reload the map to apply changes!");
ConVar portal_laser_effect("portal_laser_effect", LASER_PARTICLE, FCVAR_REPLICATED, "Effect plays on the emitter when it's active.");


class C_PropLaserEmitter : public CDynamicProp {
public:
	DECLARE_CLASS(C_PropLaserEmitter, CDynamicProp);
	DECLARE_CLIENTCLASS();

	~C_PropLaserEmitter();

	virtual void Precache();
	virtual void Spawn();
	virtual void ClientThink();
	
	void LaserOn();
	void LaserOff();

	CBeam* CreateBeam(const Vector& start, const Vector& end);
private:
	Vector m_vecBeamStart, m_vecBeamEnd, m_vecBeamPortalIn, m_vecBeamPortalOut;
	bool m_bThroughPortal, m_bStatus, m_bPrevStatus;
	CBeam* m_pBeam1;
	CBeam* m_pBeam2;
};

IMPLEMENT_CLIENTCLASS_DT(C_PropLaserEmitter, DT_PropLaserEmitter, CPropLaserEmitter)
	RecvPropVector(RECVINFO(m_vecBeamStart)),
	RecvPropVector(RECVINFO(m_vecBeamEnd)),
	RecvPropVector(RECVINFO(m_vecBeamPortalIn)),
	RecvPropVector(RECVINFO(m_vecBeamPortalOut)),
	RecvPropBool(RECVINFO(m_bThroughPortal)),
	RecvPropBool(RECVINFO(m_bStatus)),
END_RECV_TABLE()

C_PropLaserEmitter::~C_PropLaserEmitter() {
	if (m_pBeam1 != NULL) {
		m_pBeam1->Remove();
	}
	if (m_pBeam2 != NULL) {
		m_pBeam2->Remove();
	}
}

void C_PropLaserEmitter::Precache() {
	if (portal_laser_effect.GetString() != NULL) {
		PrecacheParticleSystem(portal_laser_effect.GetString());
	}

	PrecacheParticleSystem(LASER_PARTICLE);

	BaseClass::Precache();
}

void C_PropLaserEmitter::Spawn() {
	BaseClass::Spawn();

	SetThink(&C_PropLaserEmitter::ClientThink);
	SetNextClientThink(CLIENT_THINK_ALWAYS);

	m_pBeam1 = NULL;
	m_bPrevStatus = false;
}

void C_PropLaserEmitter::ClientThink() {
	// Update laser status
	if (m_bStatus) {
		LaserOn();
	} else {
		LaserOff();
	}
}

void C_PropLaserEmitter::LaserOn() {
	if (m_bThroughPortal) {
		if (m_pBeam1 == NULL) {
			m_pBeam1 = CreateBeam(m_vecBeamStart, m_vecBeamPortalIn);
		} else {
			m_pBeam1->RemoveEffects(EF_NODRAW);
			m_pBeam1->PointsInit(m_vecBeamStart, m_vecBeamPortalIn);
		}

		if (m_pBeam2 == NULL) {
			m_pBeam2 = CreateBeam(m_vecBeamPortalOut, m_vecBeamEnd);
		} else {
			m_pBeam2->RemoveEffects(EF_NODRAW);
			m_pBeam2->PointsInit(m_vecBeamPortalOut, m_vecBeamEnd);
		}
	} else {
		if (m_pBeam1 == NULL) {
			m_pBeam1 = CreateBeam(m_vecBeamStart, m_vecBeamEnd);
		}
		else {
			m_pBeam1->RemoveEffects(EF_NODRAW);
			m_pBeam1->PointsInit(m_vecBeamStart, m_vecBeamEnd);
		}

		if (m_pBeam2 != NULL) {
			m_pBeam2->AddEffects(EF_NODRAW);
		}
	}
}
void C_PropLaserEmitter::LaserOff() {
	if (m_pBeam1) {
		m_pBeam1->AddEffects(EF_NODRAW);
	}
	if (m_pBeam2) {
		m_pBeam2->AddEffects(EF_NODRAW);
	}
}

CBeam* C_PropLaserEmitter::CreateBeam(const Vector& start, const Vector& end) {
	const char* spr = portal_laser_texture.GetString();
	if (spr == NULL) {
		spr = LASER_BEAM_SPRITE;
	}

	CBeam* pBeam = CBeam::BeamCreate(spr, 1);
	
	if (pBeam) {
		int r = 0xFF, g = 0x00, b = 0x00;
		const char* colours = portal_laser_colour.GetString();
		if (colours != NULL) {
			sscanf(colours, "%i%i%i", &r, &g, &b);
		}

		pBeam->SetColor(r, g, b);
		pBeam->SetBrightness(255);
		pBeam->PointsInit(start, end);
		//pBeam->SetStartEntity(this);
		//pBeam->SetParent(this);
	}

	return pBeam;
}
