//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: The various Cores of Aperture Science.
//
//=====================================================================================//

#include "cbase.h"
#include "baseentity.h"
#include "ai_baseactor.h"
#include "props.h"				// CPhysicsProp base class
#include "saverestore_utlvector.h"

#define GLADOS_CORE_MODEL_NAME "models/npcs/personality_sphere/personality_sphere.mdl" 
// Should replace that with the non-skin version -Done

static const char *s_pAnimateThinkContext = "Animate";

// Morgan Freeman
#define SPHERE01_LOOK_ANINAME			"sphere_idle_neutral"
#define SPHERE01_LOOK_ANINAME_PLUGGED	"sphere_plug_idle_neutral"
#define SPHERE01_SKIN					0
// Aquarium
#define SPHERE02_LOOK_ANINAME			"sphere_idle_happy" //should be sphere_idle_aquarium but we don't have that
#define SPHERE02_LOOK_ANINAME_PLUGGED	"sphere_plug_idle_happy"
#define SPHERE02_SKIN					1
// Pendleton
#define SPHERE03_LOOK_ANINAME			"sphere_idle_snobby" //Pendleton fake as hell so this is a Guess
#define SPHERE03_LOOK_ANINAME_PLUGGED	"sphere_plug_idle_snobby"
#define SPHERE03_SKIN					2

// Extra Slots - Unused
//Quint
#define SPHERE04_LOOK_ANINAME			"sphere_idle_neutral"
#define SPHERE04_SKIN					3
//Wheatley
#define SPHERE05_LOOK_ANINAME			"sphere_idle_neutral"
#define SPHERE05_SKIN					4
//Wheatley - Cracked
#define SPHERE05B_LOOK_ANINAME			"sphere_damaged_idle_neutral"
#define SPHERE05B_SKIN					5
//Fact
#define SPHERE06_LOOK_ANINAME			"core01_idle"
#define SPHERE06_SKIN					6
//Space
#define SPHERE07_LOOK_ANINAME			"core02_idle"
#define SPHERE07_SKIN					7
//Adventure
#define SPHERE08_LOOK_ANINAME			"core03_idle"
#define SPHERE08_SKIN					8

class CPropPersonalityCore : public CPhysicsProp
{
public:
	DECLARE_CLASS( CPropPersonalityCore, CPhysicsProp );
	DECLARE_DATADESC();

	CPropPersonalityCore();
	~CPropPersonalityCore();

	typedef enum 
	{
		CORETYPE_SPHERE01,
		CORETYPE_SPHERE02,
		CORETYPE_SPHERE03,
		CORETYPE_PROTO01,
		CORETYPE_PROTO02,
		CORETYPE_PROTO02B,
		CORETYPE_CORE01,
		CORETYPE_CORE02,
		CORETYPE_CORE03,
		CORETYPE_NONE,

	} CORETYPE;

	typedef enum
	{
		HELD,
		NOTHELD,
		UNHELD,

	} VOMODE;

	virtual void Spawn( void );
	virtual void Precache( void );

	virtual int		OnTakeDamage(const CTakeDamageInfo &info);
	virtual QAngle	PreferredCarryAngles( void ) { return QAngle( 180, -90, 180 ); }
	virtual bool	HasPreferredCarryAnglesForPlayer( CBasePlayer *pPlayer ) { return true; }

	void	InputStartTalking(inputdata_t& inputdata);
	void	PlayAttach(inputdata_t& inputdata);
	void	PlayDettach(inputdata_t& inputdata);

	void	StartTalking(float flDelay);
	void	TalkingThink(void);
	void	AnimateThink ( void );

	void	SetupVOList(void);
	
	void	OnPhysGunPickup( CBasePlayer* pPhysGunUser, PhysGunPickup_t reason );
	void	OnPhysGunDrop(CBasePlayer* pPhysGunUser, PhysGunDrop_t reason);

private:
	int m_iTotalLines;
	float m_flBetweenVOPadding;		// Spacing (in seconds) between VOs
	bool m_bFirstPickup;
	bool m_bIsAttached;

	// Names of sound scripts for this core's personality
	CUtlVector<string_t> m_speechEvents;
	int m_iSpeechIter;

	string_t	m_iszLookAnimationName;		// Different animations for each personality
	string_t	m_iszGruntSoundScriptName;
	//string_t	m_iszPlugAnimationName = AllocPooledString(SPHERE01_LOOK_ANINAME);

	VOMODE m_iVoMode;
	CORETYPE m_iCoreType;
};

LINK_ENTITY_TO_CLASS( prop_personality_core, CPropPersonalityCore );

//-----------------------------------------------------------------------------
// Save/load 
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CPropPersonalityCore )

	DEFINE_FIELD(m_iTotalLines, FIELD_INTEGER),
	DEFINE_FIELD(m_iSpeechIter, FIELD_INTEGER),
	DEFINE_FIELD( m_iszLookAnimationName,					FIELD_STRING ),
	DEFINE_UTLVECTOR(m_speechEvents, FIELD_STRING),
	DEFINE_FIELD(m_iszGruntSoundScriptName, FIELD_STRING),
	DEFINE_FIELD( m_bFirstPickup,							FIELD_BOOLEAN ),

	DEFINE_KEYFIELD( m_iVoMode,			FIELD_INTEGER, "VoMode"),
	DEFINE_KEYFIELD( m_iCoreType,			FIELD_INTEGER, "CoreType" ),
	DEFINE_KEYFIELD(m_flBetweenVOPadding, FIELD_FLOAT, "DelayBetweenLines"),

	DEFINE_INPUTFUNC(FIELD_VOID, "StartTalking", InputStartTalking),
	DEFINE_INPUTFUNC(FIELD_VOID, "PlayAttach", PlayAttach),
	DEFINE_INPUTFUNC(FIELD_VOID, "PlayDettach", PlayDettach),
	
	DEFINE_THINKFUNC(TalkingThink),
	DEFINE_THINKFUNC( AnimateThink ),
	
END_DATADESC()

CPropPersonalityCore::CPropPersonalityCore()
{
	m_iTotalLines = m_iSpeechIter = 0;
	m_flBetweenVOPadding = 2.5f;
	m_bFirstPickup = true;
}

CPropPersonalityCore::~CPropPersonalityCore()
{
	m_speechEvents.Purge();
}

void CPropPersonalityCore::Spawn( void )
{
	m_iVoMode = NOTHELD;

	SetupVOList();

	Precache();
	KeyValue( "model", GLADOS_CORE_MODEL_NAME );
	BaseClass::Spawn();

	//Default to 'dropped' animation
	ResetSequence(LookupSequence(STRING(m_iszLookAnimationName)));
	SetCycle(1.0f);

	DisableAutoFade();

	SetContextThink( &CPropPersonalityCore::AnimateThink, gpGlobals->curtime + 0.1f, s_pAnimateThinkContext );
}

void CPropPersonalityCore::Precache( void )
{
	BaseClass::Precache();
	
	//TEMP VO
	PrecacheScriptSound("citadel.br_youneedme");
	PrecacheScriptSound("npc_citizen.doingsomething");
	PrecacheScriptSound("npc_citizen.holddownspot02");
	PrecacheScriptSound("npc_citizen.die");

	PrecacheModel( GLADOS_CORE_MODEL_NAME );
}

//-----------------------------------------------------------------------------
// Purpose: Start playing personality VO list
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CPropPersonalityCore::InputStartTalking(inputdata_t &inputdata)
{
	StartTalking(0.0f);
}

void CPropPersonalityCore::StartTalking(float flDelay)
{
	if (m_speechEvents.IsValidIndex(m_iSpeechIter) && m_speechEvents.Count() > 0)
	{
		StopSound(m_speechEvents[m_iSpeechIter].ToCStr());
	}

	m_iSpeechIter = 0;
	SetThink(&CPropPersonalityCore::TalkingThink);
	SetNextThink(gpGlobals->curtime + m_flBetweenVOPadding + flDelay);
}

//-----------------------------------------------------------------------------
// Purpose: Start playing personality VO list
//-----------------------------------------------------------------------------
void CPropPersonalityCore::TalkingThink(void)
{
	if (m_speechEvents.Count() <= 0 || !m_speechEvents.IsValidIndex(m_iSpeechIter))
	{
		SetThink(NULL);
		SetNextThink(gpGlobals->curtime);
		return;
	}

	// Loop the 'look around' animation after the first line.
	int iCurSequence = GetSequence();
	int iLookSequence = LookupSequence(STRING(m_iszLookAnimationName));
	if (iCurSequence != iLookSequence && m_iSpeechIter > 0)
	{
		ResetSequence(iLookSequence);
	}

	int iPrevIter = m_iSpeechIter - 1;
	if (iPrevIter < 0)
		iPrevIter = 0;

	StopSound(m_speechEvents[iPrevIter].ToCStr());

	float flCurDuration = GetSoundDuration(m_speechEvents[m_iSpeechIter].ToCStr(), GLADOS_CORE_MODEL_NAME);

	EmitSound(m_speechEvents[m_iSpeechIter].ToCStr());
	SetNextThink(gpGlobals->curtime + m_flBetweenVOPadding + flCurDuration);

	// wrap if we hit the end of the list
	m_iSpeechIter = (m_iSpeechIter + 1) % m_speechEvents.Count();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
//-----------------------------------------------------------------------------
void CPropPersonalityCore::AnimateThink()
{
	StudioFrameAdvance();
	SetContextThink( &CPropPersonalityCore::AnimateThink, gpGlobals->curtime + 0.1f, s_pAnimateThinkContext );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
//-----------------------------------------------------------------------------
void CPropPersonalityCore::SetupVOList(void)
{
	switch (m_iCoreType)
	{
	case CORETYPE_SPHERE01:
	{
		switch (m_iVoMode)
		{
		case HELD:
		{
			m_speechEvents.Purge();
			m_speechEvents.AddToTail(AllocPooledString("citadel.br_youneedme"));
		}
		break;
		case NOTHELD:
		{
			m_speechEvents.Purge();
			m_speechEvents.AddToTail(AllocPooledString("npc_citizen.doingsomething"));
		}
		break;
		case UNHELD:
		{
			m_speechEvents.Purge();
			m_speechEvents.AddToTail(AllocPooledString("npc_citizen.holddownspot02"));
		}
		break;
		}

		m_iszGruntSoundScriptName = AllocPooledString("npc_citizen.die");
		m_iszLookAnimationName = AllocPooledString(SPHERE01_LOOK_ANINAME);
		m_nSkin = SPHERE01_SKIN;

	}
	break;
	case CORETYPE_SPHERE02:
	{
		switch (m_iVoMode)
		{
		case HELD:
		{
			m_speechEvents.Purge();
			m_speechEvents.AddToTail(AllocPooledString("citadel.br_youneedme"));
		}
		break;
		case NOTHELD:
		{
			m_speechEvents.Purge();
			m_speechEvents.AddToTail(AllocPooledString("npc_citizen.doingsomething"));
		}
		break;
		case UNHELD:
		{
			m_speechEvents.Purge();
			m_speechEvents.AddToTail(AllocPooledString("npc_citizen.holddownspot02"));
		}
		break;
		}

		m_iszGruntSoundScriptName = AllocPooledString("npc_citizen.die");
		m_iszLookAnimationName = AllocPooledString(SPHERE02_LOOK_ANINAME);
		m_nSkin = SPHERE02_SKIN;

	}
	break;
	case CORETYPE_SPHERE03:
	{
		switch (m_iVoMode)
		{
		case HELD:
		{
			m_speechEvents.Purge();
			m_speechEvents.AddToTail(AllocPooledString("citadel.br_youneedme"));
		}
		break;
		case NOTHELD:
		{
			m_speechEvents.Purge();
			m_speechEvents.AddToTail(AllocPooledString("npc_citizen.doingsomething"));
		}
		break;
		case UNHELD:
		{
			m_speechEvents.Purge();
			m_speechEvents.AddToTail(AllocPooledString("npc_citizen.holddownspot02"));
		}
		break;
		}

		m_iszGruntSoundScriptName = AllocPooledString("npc_citizen.die");
		m_iszLookAnimationName = AllocPooledString(SPHERE03_LOOK_ANINAME);
		m_nSkin = SPHERE03_SKIN;

	}
	break;
	case CORETYPE_PROTO01:
	{
		m_speechEvents.AddToTail(AllocPooledString("common/null.wav"));
		m_iszGruntSoundScriptName = AllocPooledString("common/null.wav");
		m_iszLookAnimationName = AllocPooledString(SPHERE04_LOOK_ANINAME);
		m_nSkin = SPHERE04_SKIN;
	}
	break;
	case CORETYPE_PROTO02:
	{
		m_speechEvents.AddToTail(AllocPooledString("common/null.wav"));
		m_iszGruntSoundScriptName = AllocPooledString("common/null.wav");
		m_iszLookAnimationName = AllocPooledString(SPHERE05_LOOK_ANINAME);
		m_nSkin = SPHERE05_SKIN;
	}
	break;
	case CORETYPE_PROTO02B:
	{
		m_speechEvents.AddToTail(AllocPooledString("common/null.wav"));
		m_iszGruntSoundScriptName = AllocPooledString("common/null.wav");
		m_iszLookAnimationName = AllocPooledString(SPHERE05B_LOOK_ANINAME);
		m_nSkin = SPHERE05B_SKIN;
	}
	break;
	case CORETYPE_CORE01:
	{
		m_speechEvents.AddToTail(AllocPooledString("common/null.wav"));
		m_iszGruntSoundScriptName = AllocPooledString("common/null.wav");
		m_iszLookAnimationName = AllocPooledString(SPHERE06_LOOK_ANINAME);
		m_nSkin = SPHERE06_SKIN;
	}
	break;
	case CORETYPE_CORE02:
	{
		m_speechEvents.AddToTail(AllocPooledString("common/null.wav"));
		m_iszGruntSoundScriptName = AllocPooledString("common/null.wav");
		m_iszLookAnimationName = AllocPooledString(SPHERE07_LOOK_ANINAME);
		m_nSkin = SPHERE07_SKIN;
	}
	break;
	case CORETYPE_CORE03:
	{
		m_speechEvents.AddToTail(AllocPooledString("common/null.wav"));
		m_iszGruntSoundScriptName = AllocPooledString("common/null.wav");
		m_iszLookAnimationName = AllocPooledString(SPHERE08_LOOK_ANINAME);
		m_nSkin = SPHERE08_SKIN;
	}
	break;
	default:
	{
		switch (m_iVoMode)
		{
		case HELD:
		{
			m_speechEvents.Purge();
			m_speechEvents.AddToTail(AllocPooledString("common/null.wav"));
		}
		break;
		case NOTHELD:
		{
			m_speechEvents.Purge();
			m_speechEvents.AddToTail(AllocPooledString("common/null.wav"));
		}
		break;
		case UNHELD:
		{
			m_speechEvents.Purge();
			m_speechEvents.AddToTail(AllocPooledString("common/null.wav"));
		}
		break;
		}

		m_iszGruntSoundScriptName = AllocPooledString("common/null.wav");
		m_iszLookAnimationName = AllocPooledString(SPHERE01_LOOK_ANINAME);
		m_nSkin = SPHERE01_SKIN;
	}
	break;
	};
}

void CPropPersonalityCore::PlayAttach(inputdata_t &inputdata)
{
	m_bIsAttached = true;

	switch (m_iCoreType)
	{
	case CORETYPE_SPHERE01:
	{
		m_iszLookAnimationName = AllocPooledString(SPHERE01_LOOK_ANINAME_PLUGGED);
	}
	break;
	case CORETYPE_SPHERE02:
	{
		m_iszLookAnimationName = AllocPooledString(SPHERE02_LOOK_ANINAME_PLUGGED);
	}
	break;
	case CORETYPE_SPHERE03:
	{
		m_iszLookAnimationName = AllocPooledString(SPHERE03_LOOK_ANINAME_PLUGGED);
	}
	break;
	}
	ResetSequence(LookupSequence("sphere_plug_attach"));
	SetupVOList();
}

void CPropPersonalityCore::PlayDettach(inputdata_t &inputdata)
{
	m_bIsAttached = false;

	switch (m_iCoreType)
	{
	case CORETYPE_SPHERE01:
	{
		m_iszLookAnimationName = AllocPooledString(SPHERE01_LOOK_ANINAME);
	}
	break;
	case CORETYPE_SPHERE02:
	{
		m_iszLookAnimationName = AllocPooledString(SPHERE02_LOOK_ANINAME);
	}
	break;
	case CORETYPE_SPHERE03:
	{
		m_iszLookAnimationName = AllocPooledString(SPHERE03_LOOK_ANINAME);
	}
	break;
	}
	ResetSequence(LookupSequence("sphere_plug_attach"));
	SetupVOList();
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
//-----------------------------------------------------------------------------
void CPropPersonalityCore::OnPhysGunPickup( CBasePlayer* pPhysGunUser, PhysGunPickup_t reason )
{
	m_iVoMode = HELD;
	//StopSound(m_speechEvents[m_iSpeechIter].ToCStr());
	SetupVOList();
	//EmitSound(m_speechEvents[m_iSpeechIter].ToCStr());
	float flTalkingDelay = (2.0f);
	StartTalking(flTalkingDelay);

	// +use always enables motion on these props
	EnableMotion();

	BaseClass::OnPhysGunPickup ( pPhysGunUser, reason );
}

void CPropPersonalityCore::OnPhysGunDrop(CBasePlayer* pPhysGunUser, PhysGunDrop_t reason)
{
	m_iVoMode = UNHELD;
	//StopSound(m_speechEvents[m_iSpeechIter].ToCStr());
	SetupVOList();
	//EmitSound(m_speechEvents[m_iSpeechIter].ToCStr());
	float flTalkingDelay = (2.0f);
	StartTalking(flTalkingDelay);

	// +use always enables motion on these props
	EnableMotion();

	BaseClass::OnPhysGunDrop(pPhysGunUser, reason);
}

int CPropPersonalityCore::OnTakeDamage(const CTakeDamageInfo &info)
{
	StopSound(m_speechEvents[m_iSpeechIter].ToCStr());
	EmitSound(m_iszGruntSoundScriptName.ToCStr());
	float flTalkingDelay = (2.0f);
	StartTalking(flTalkingDelay);

	BaseClass::OnTakeDamage(info);

	if (info.GetDamageType() & DMG_CRUSH & DMG_DIRECT & DMG_GENERIC)
		return BaseClass::OnTakeDamage(info);

	return 0;
}