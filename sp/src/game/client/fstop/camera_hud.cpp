#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "c_basehlplayer.h" //alternative #include "c_baseplayer.h"

#include <vgui/IScheme.h>
#include <vgui_controls/Panel.h>

#include "camera_hud.h"

// memdbgon must be the last include file in a .cpp file!
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT(CCameraHUD);
DECLARE_HUD_MESSAGE(CCameraHUD, ShowHud);

using namespace vgui;

/**
 * Constructor - generic HUD element initialization stuff. Make sure our 2 member variables
 * are instantiated.
 */
CCameraHUD::CCameraHUD(const char* pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudCamera")
{
	vgui::Panel* pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	m_bShow = false;
	m_pHud = 0;

	// Scope will not show when the player is dead
	SetHiddenBits(HIDEHUD_PLAYERDEAD);

	// fix for users with diffrent screen ratio (Lodle)
	int screenWide, screenTall;
	GetHudSize(screenWide, screenTall);
	SetBounds(0, 0, screenWide, screenTall);
}

/**
 * Hook up our HUD message, and make sure we are not showing the scope
 */
void CCameraHUD::Init()
{
	HOOK_HUD_MESSAGE(CCameraHUD, ShowHud);

	m_bShow = false;
}

/**
 * Load  in the scope material here
 */
void CCameraHUD::ApplySchemeSettings(vgui::IScheme* scheme)
{
	BaseClass::ApplySchemeSettings(scheme);

	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);

	if (!m_pHud)
	{
		m_pHud = gHUD.GetIcon("scope");
	}
}

/**
 * Simple - if we want to show the scope, draw it. Otherwise don't.
 */
void CCameraHUD::Paint(void)
{
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
	{
		return;
	}

	if (m_bShow)
	{
		//Perform depth hack to prevent clips by world
	//materials->DepthRange( 0.0f, 0.1f );

// This will draw the scope at the origin of this HUD element, and
// stretch it to the width and height of the element. As long as the
// HUD element is set up to cover the entire screen, so will the scope
		m_pHud->DrawSelf(0, 0, GetWide(), GetTall(), Color(255, 255, 255, 255));

		//Restore depth
		//materials->DepthRange( 0.0f, 1.0f );

	// Hide the crosshair
		pPlayer->m_Local.m_iHideHUD |= HIDEHUD_CROSSHAIR;
	}
	else if ((pPlayer->m_Local.m_iHideHUD & HIDEHUD_CROSSHAIR) != 0)
	{
		pPlayer->m_Local.m_iHideHUD &= ~HIDEHUD_CROSSHAIR;
	}
}


/**
 * Callback for our message - set the show variable to whatever
 * boolean value is received in the message
 */
void CCameraHUD::MsgFunc_ShowHud(bf_read& msg)
{
	m_bShow = msg.ReadByte();
}