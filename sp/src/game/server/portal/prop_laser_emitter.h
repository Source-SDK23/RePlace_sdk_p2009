#pragma once

#include "cbase.h"
#include "props.h"

#include "prop_laser_catcher.h"

class CPropLaserEmitter : public CDynamicProp {
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
private:
	CBeam* m_pBeam;
	bool m_bStatus, m_bStartActive;

	CSoundPatch* m_pLaserSound;
	CParticleSystem* m_pLaserFx;

	CFuncLaserDetector* m_pCatcher;

	float m_fPainTimer;

	DECLARE_DATADESC();
};