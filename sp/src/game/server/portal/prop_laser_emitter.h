#pragma once

#include "cbase.h"
#include "props.h"
#include "Sprite.h"

#include "prop_laser_catcher.h"
#include "prop_weightedcube.h"
#include "prop_schrodingercube.h"

/**
 * @brief Thermal Discouragement Beam Entity
*/
class CEnvPortalLaser : public CBaseEntity {
public:
	DECLARE_CLASS(CEnvPortalLaser, CBaseEntity);
	DECLARE_DATADESC();

	CEnvPortalLaser();
	~CEnvPortalLaser();

	virtual void Precache();
	virtual void Spawn();

	void LaserThink();

	void TurnOn();
	void TurnOff();
	void Toggle();
	
	bool GetState();

	void CreateSounds();
	void DestroySounds();

	void SetBeamColour(byte r, byte g, byte b);
	void SetSpriteColour(byte r, byte g, byte b);

	void InputTurnOn(inputdata_t& data);
	void InputTurnOff(inputdata_t& data);
	void InputToggle(inputdata_t& data);

	void HandlePlayerKnockback(const Vector& vecDir, const Vector& vecStart, const Vector& vecEnd);

	void SetPlayerDamage(float damage);
private:
	CBeam* m_pBeam;
	CBeam* m_pBeamAfterPortal;
	bool m_bStartActive;
	bool m_bStatus;

	CSoundPatch* m_pLaserSound;

	CFuncLaserDetector* m_pCatcher;
	CPropWeightedCube* m_pCube;
	CPropSchrodingerCube* m_pSCube;

	CNetworkColor32(m_clrBeam);
	CNetworkColor32(m_clrSprite);

	float m_fHurnSoundTime, m_fPlayerDamage;
	int m_iSprite;
};

class CPropLaserEmitter : public CDynamicProp {
public:
	DECLARE_CLASS(CPropLaserEmitter, CDynamicProp);

	CPropLaserEmitter();
	~CPropLaserEmitter();

	virtual void Precache();
	virtual void Spawn();

	void TurnOn();
	void TurnOff();
	void Toggle();

	void InputTurnOn(inputdata_t& data);
	void InputTurnOff(inputdata_t& data);
	void InputToggle(inputdata_t& data);
private:
	CEnvPortalLaser* m_pLaser;
	CSprite* m_pLaserSprite;

	CNetworkColor32(m_clrBeam);
	CNetworkColor32(m_clrSprite);
	float m_fPlayerDamage;

	DECLARE_DATADESC();

	bool m_bStartActive;
};