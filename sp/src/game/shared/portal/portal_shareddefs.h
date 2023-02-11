//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef PORTAL_SHAREDDEFS_H
#define PORTAL_SHAREDDEFS_H
#ifdef _WIN32
#pragma once
#endif


//#define PORTAL_HALF_WIDTH 32.0f
//#define PORTAL_HALF_HEIGHT 54.0f
#define PORTAL_DEFAULT_HALF_WIDTH 32.0f
#define PORTAL_DEFAULT_HALF_HEIGHT 54.0f
#define PORTAL_HALF_DEPTH 2.0f
#define PORTAL_BUMP_FORGIVENESS 2.0f

#define PORTAL_ANALOG_SUCCESS_NO_BUMP 1.0f
#define PORTAL_ANALOG_SUCCESS_BUMPED 0.3f
#define PORTAL_ANALOG_SUCCESS_CANT_FIT 0.1f
#define PORTAL_ANALOG_SUCCESS_CLEANSER 0.028f
#define PORTAL_ANALOG_SUCCESS_OVERLAP_LINKED 0.027f
#define PORTAL_ANALOG_SUCCESS_NEAR 0.0265f
#define PORTAL_ANALOG_SUCCESS_INVALID_VOLUME 0.026f
#define PORTAL_ANALOG_SUCCESS_INVALID_SURFACE 0.025f
#define PORTAL_ANALOG_SUCCESS_PASSTHROUGH_SURFACE 0.0f

#define MIN_FLING_SPEED 300

#define PORTAL_HIDE_PLAYER_RAGDOLL 1

enum PortalFizzleType_t
{
	PORTAL_FIZZLE_SUCCESS = 0,			// Placed fine (no fizzle)
	PORTAL_FIZZLE_CANT_FIT,
	PORTAL_FIZZLE_OVERLAPPED_LINKED,
	PORTAL_FIZZLE_BAD_VOLUME,
	PORTAL_FIZZLE_BAD_SURFACE,
	PORTAL_FIZZLE_KILLED,
	PORTAL_FIZZLE_CLEANSER,
	PORTAL_FIZZLE_CLOSE,
	PORTAL_FIZZLE_NEAR_BLUE,
	PORTAL_FIZZLE_NEAR_RED,
	PORTAL_FIZZLE_NONE,

	NUM_PORTAL_FIZZLE_TYPES
};


enum PortalPlacedByType
{
	PORTAL_PLACED_BY_FIXED = 0,
	PORTAL_PLACED_BY_PEDESTAL,
	PORTAL_PLACED_BY_PLAYER
};

enum PortalLevelStatType
{
	PORTAL_LEVEL_STAT_NUM_PORTALS = 0,
	PORTAL_LEVEL_STAT_NUM_STEPS,
	PORTAL_LEVEL_STAT_NUM_SECONDS,

	PORTAL_LEVEL_STAT_TOTAL
};

enum PortalChallengeType
{
	PORTAL_CHALLENGE_NONE = 0,
	PORTAL_CHALLENGE_PORTALS,
	PORTAL_CHALLENGE_STEPS,
	PORTAL_CHALLENGE_TIME,

	PORTAL_CHALLENGE_TOTAL
};

extern char *g_ppszPortalPassThroughMaterials[];

#endif // PORTAL_SHAREDDEFS_H
