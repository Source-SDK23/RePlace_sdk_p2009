#pragma once

#include "cbase.h"
#include "props.h"

#include "prop_laser_catcher.h"

class CPropLaserEmitter : public CDynamicProp {
	DECLARE_SERVERCLASS();
public:
	DECLARE_CLASS(CPropLaserEmitter, CDynamicProp);

	CPropLaserEmitter();
	~CPropLaserEmitter();

	virtual void Precache();
	virtual void Spawn();

	void LaserThink();

	void TurnOn();
	void TurnOff();
	void Toggle();

	void CreateSounds();
	void DestroySounds();

	void InputTurnOn(inputdata_t& data);
	void InputTurnOff(inputdata_t& data);
	void InputToggle(inputdata_t& data);

	static CPropLaserEmitter* Create(const Vector& origin, const QAngle& angle, const char* propModel, CBaseEntity* parent);
	static CPropLaserEmitter* CreateNoModel(const Vector& origin, const QAngle& angle, CBaseEntity* parent);
private:
	CBeam* m_pBeam;
	bool m_bStartActive;
	
	CNetworkVar(bool, m_bStatus);
	CNetworkVar(bool, m_bThroughPortal);

	CSoundPatch* m_pLaserSound;
	//CParticleSystem* m_pLaserFx;

	CFuncLaserDetector* m_pCatcher;
	CTraceFilterSkipTwoEntities m_BeamFilter;

	float m_fPainTimer;

	CNetworkVar(Vector, m_vecBeamStart);
	CNetworkVar(Vector, m_vecBeamEnd);
	CNetworkVar(Vector, m_vecBeamPortalIn);
	CNetworkVar(Vector, m_vecBeamPortalOut);

	DECLARE_DATADESC();
};