// =============== Fortress Forever ==============
// ======== A modification for Half-Life 2 =======
//
// @file ff_buildableinfo.h
// @author Patrick O'Leary (Mulchman)
// @date ?/?/2005
// @brief BuildableInfo class
//
// ===============================================

#include "cbase.h"
#include "ff_buildableinfo.h"

#ifdef CLIENT_DLL
	#include "c_ff_player.h"
#elif GAME_DLL
	#include "ff_player.h"
	#include "omnibot_interface.h"
#endif

//-----------------------------------------------------------------------------
// Purpose: Returns a proper angle orientation for the buidlable depending
//			on the approximate ground normal
//-----------------------------------------------------------------------------
QAngle OrientToVectors(const Vector &vecGroundNormal, const Vector &vecPlayerForward) 
{
	// Get correct forward & right vector
	Vector vecUp = vecGroundNormal;
	Vector vecRight = CrossProduct(vecUp, -vecPlayerForward);
	Vector vecForward = CrossProduct(vecUp, vecRight);

	// Normalise(jic) 
	VectorNormalize(vecForward);
	VectorNormalize(vecRight);
	VectorNormalize(vecUp);

	// Quaternions are the handiest things
	Quaternion q;
	BasisToQuaternion(vecForward, vecRight, vecUp, q);

	// Now for the angles
	QAngle angNew;
	QuaternionAngles(q, angNew);

	return angNew;
}

//-----------------------------------------------------------------------------
// Purpose: Vectors must be	0 1
//							2 3
//-----------------------------------------------------------------------------
void ComputeRectangularPlane(const Vector &v0, const Vector &v1, const Vector &v2, const Vector &v3, Vector &vOut) 
{
	Vector vecOne = v0 - v3;
	Vector vecTwo = v2 - v1;

	VectorNormalize(vecOne);
	VectorNormalize(vecTwo);

	vOut = CrossProduct(vecOne, vecTwo);

	VectorNormalize(vOut);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector GetMiddle(const Vector *v1, const Vector *v2, const Vector *v3 = NULL, const Vector *v4 = NULL) 
{
	float c = 2.0f;
	Vector vecMiddle = *v1 + *v2;

	if (v3) 
	{
		vecMiddle += *v3;
		c++;
	}
	if (v4) 
	{
		vecMiddle += *v4;
		c++;
	}

	vecMiddle /= c;

	return vecMiddle;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor - initializes a bunch of stuff and figures out if
//			we can build here or not!
//-----------------------------------------------------------------------------
CFFBuildableInfo::CFFBuildableInfo( CFFPlayer *pPlayer, int iBuildObject ) 
{
	// Default
	m_BuildResult = BUILD_ERROR;
	
	// Ack, no player?
	if( !pPlayer )
	{
		Assert( 0 );
		m_BuildResult = BUILD_NOPLAYER;		
		return;
	}

	// Set up some build parameters
	float flBuildDist = 0.0f, flOffset = 0.0f;
	switch( iBuildObject )
	{
		case FF_BUILD_DISPENSER: flBuildDist = FF_BUILD_DISP_BUILD_DIST; flOffset = -22.0f; break;
		case FF_BUILD_SENTRYGUN: flBuildDist = FF_BUILD_SG_BUILD_DIST; flOffset = -22.0f; break;
		case FF_BUILD_DETPACK: flBuildDist = FF_BUILD_DET_BUILD_DIST; break;
		case FF_BUILD_MANCANNON: flBuildDist = FF_BUILD_MC_BUILD_DIST; break;
	}

	// Player building the object
	m_pPlayer = pPlayer;
	// Object we're building
	m_iBuildObject = iBuildObject;
	// Distance in front of player to build
	m_flBuildDist = flBuildDist;

	// Check ducking
	if( m_pPlayer->GetFlags() & FL_DUCKING )
		flOffset /= 2.0f;

	// Get some info from the player...
	m_pPlayer->EyeVectors( &m_vecPlayerForward, &m_vecPlayerRight );

	// Level off
	m_vecPlayerForward.z = 0;
	m_vecPlayerRight.z = 0;

	// Normalize
	VectorNormalize( m_vecPlayerForward );
	VectorNormalize( m_vecPlayerRight );

	// Store player origin (offsetted)
	m_vecPlayerOrigin = m_pPlayer->GetAbsOrigin() + ( m_vecPlayerForward * 16.0f ) + Vector( 0, 0, flOffset );

	// Get a position in front of the player a certain distance & raise it up.
	// This is the first position we'll try to build from (offsetted)
	m_vecBuildAirOrigin = m_vecPlayerOrigin + ( m_vecPlayerForward * m_flBuildDist );	

	// Original angle is same as player's
	VectorAngles( m_vecPlayerForward, m_angBuildAirAngles );

	// Set these for the build helpers in case building fails the client
	// side helper still needs to draw an icon using valid positions
	m_vecBuildGroundOrigin = m_vecBuildAirOrigin;
	m_angBuildGroundAngles = m_angBuildAirAngles;

	switch( iBuildObject )
	{
	case FF_BUILD_DISPENSER:
		if(pPlayer->GetDispenser())
			m_BuildResult = BUILD_ALREADYBUILT;
		else if(pPlayer->GetAmmoCount( AMMO_CELLS ) < FF_BUILDCOST_DISPENSER)
			m_BuildResult = BUILD_NEEDAMMO;
		else
			break;
		return;
	case FF_BUILD_SENTRYGUN: 
		if(pPlayer->GetSentryGun())
			m_BuildResult = BUILD_ALREADYBUILT;
		else if(pPlayer->GetAmmoCount( AMMO_CELLS ) < FF_BUILDCOST_SENTRYGUN)
			m_BuildResult = BUILD_NEEDAMMO;
		else
			break;
		return;		
	case FF_BUILD_DETPACK: 
		if(pPlayer->GetDetpack())
			m_BuildResult = BUILD_ALREADYBUILT;
		else if(pPlayer->GetAmmoCount( AMMO_DETPACK ) < 1)
			m_BuildResult = BUILD_NEEDAMMO;
		else
			break;
		return;
	case FF_BUILD_MANCANNON:
		if( pPlayer->GetManCannon() )
			m_BuildResult = BUILD_ALREADYBUILT;
		else if( pPlayer->GetAmmoCount( AMMO_MANCANNON ) < 1 )
			m_BuildResult = BUILD_NEEDAMMO;
		else
			break;
	}

	// For the sg, because its so large, the hull (when doing the trace later) will stick
	// into walls if we are near them [like having our back towards them] so we need to
	// move the build origin further forward to remedy this
	if( iBuildObject == FF_BUILD_SENTRYGUN || iBuildObject == FF_BUILD_MANCANNON )
	{
		// Move forward an arbitrary amount... but don't go more
		// than 32 otherwise that leaves a gap between the players
		// bbox and the object you're building so really thin brushes
		// could be in the way and you could technically build 'through' them.
		m_vecPlayerOrigin += m_vecPlayerForward * 32.0f;
	}

	// Check if player is even on the ground or not
	if( !( m_pPlayer->GetFlags() & FL_ONGROUND ) )
	{
		m_BuildResult = BUILD_PLAYEROFFGROUND;
		return;
	}

	// Assume stuff is in the way. Next function can
	// set other build errors than just one, that's why
	// we're setting NOROOM now - in case we don't trickle
	// into one of the other errors and we do error.
	m_BuildResult = BUILD_NOROOM;

	// If we can't trace the hull
	if( IsGeometryInTheWay() )
		return;

	// If we're dealing w/ a detpack then we're finished here
	if( (m_iBuildObject == FF_BUILD_DETPACK) || (m_iBuildObject == FF_BUILD_MANCANNON) )
	{
		m_BuildResult = BUILD_ALLOWED;
		return;
	}

	// Test to see if can be oriented
	m_BuildResult = CanOrientToGround();
}

//-----------------------------------------------------------------------------
// Purpose: Checks for geometry/players where we want to build
//-----------------------------------------------------------------------------
bool CFFBuildableInfo::IsGeometryInTheWay( void )
{
	// Get the correct mins/maxs
	Vector vecMins = Vector(0, 0, 0);
	Vector vecMaxs = Vector(0, 0, 0);
	
	switch( m_iBuildObject )
	{
		case FF_BUILD_DISPENSER: vecMins = FF_DISPENSER_MINS; vecMaxs = FF_DISPENSER_MAXS; break;
		case FF_BUILD_SENTRYGUN: vecMins = FF_SENTRYGUN_MINS; vecMaxs = FF_SENTRYGUN_MAXS; break;
		case FF_BUILD_DETPACK:   vecMins = FF_DETPACK_MINS;   vecMaxs = FF_DETPACK_MAXS;   break;
		case FF_BUILD_MANCANNON: vecMins = FF_MANCANNON_MINS; vecMaxs = FF_MANCANNON_MAXS; break;
	}

	// We're going to do this test 3 times... building on an incline always kills us
	bool bValid = false;

	// Detpack was climbing up short walls and stuff, nice! But a bug!
	int iIterations = ( (m_iBuildObject == FF_BUILD_DETPACK) || (m_iBuildObject == FF_BUILD_MANCANNON) ) ? 1 : 3;

	for( int i = 0; ( i < iIterations ) && !bValid; i++ )
	{
		// If past the first iteration, try adjusting
		// player origin if building isn't working
		if( i > 0 )
		{
			m_vecPlayerOrigin.z += 16.0f;
		}

#ifdef FF_BUILD_DEBUG_VISUALIZATIONS
#ifdef GAME_DLL
#ifdef _DEBUG
		// Some visualizations
		if( !engine->IsDedicatedServer() )
		{
			// Show the players hull in yellow
			NDebugOverlay::Box( m_pPlayer->GetFeetOrigin(), Vector( -16, -16, 0 ), Vector( 16, 16, 70 ), 255, 255, 0, 100, 10.0f );
			// Show the hull in blue at the players origin
			NDebugOverlay::Box( m_vecPlayerOrigin, vecMins, vecMaxs, 0, 0, 255, 100, 10.0f );
			// Show the hull in red at the build origin
			NDebugOverlay::Box( m_vecBuildAirOrigin, vecMins, vecMaxs, 255, 0, 0, 100, 10.0f );
		}
#endif // _DEBUG
#endif // GAME_DLL
#endif // FF_BUILD_DEBUG_VISUALIZATIONS

		// Now, using the mins/maxs, do a trace
		// If the trace does not complete them we hit something and the area is not clear
		trace_t trHull;
		UTIL_TraceHull( m_vecPlayerOrigin, m_vecBuildAirOrigin, vecMins, vecMaxs, MASK_PLAYERSOLID, m_pPlayer, COLLISION_GROUP_BUILDABLE_BUILDING, &trHull );

		// See if we started in a solid
		if( trHull.startsolid )
		{
			// Raise build origin
			m_vecBuildAirOrigin.z += 16.0f;
			continue;
		}
		// See if the trace completed - if fraction == 1.0 then it completed
		else if( trHull.fraction != 1.0 )
		{
			// Raise build origin
			m_vecBuildAirOrigin.z += 16.0f;
			continue;
		}

		// Jiggles: Added to prevent people from building through certain displacements
		trace_t	evilDisp;
		UTIL_TraceLine( m_pPlayer->GetAbsOrigin(), m_vecBuildAirOrigin, MASK_PLAYERSOLID, m_pPlayer, COLLISION_GROUP_BUILDABLE_BUILDING, &evilDisp );
		if ( evilDisp.fraction != 1.0 )
			continue;

		bValid = true;
	}

	// Stuff was constantly blocking us
	if( !bValid )
	{
		m_BuildResult = BUILD_NOROOM;
		return true;	
	}

	// Can't build while standing on ele or object is on ele
	trace_t tr1;
	UTIL_TraceLine( m_vecPlayerOrigin, m_vecPlayerOrigin - Vector( 0, 0, 64 ), CONTENTS_MOVEABLE | CONTENTS_SOLID, m_pPlayer, COLLISION_GROUP_PLAYER, &tr1 );
	if( tr1.DidHit() && tr1.m_pEnt )
	{
		if( tr1.m_pEnt->GetMoveType() == MOVETYPE_PUSH )
		{
			m_BuildResult = BUILD_MOVEABLE;
			return true;
		}
	}

	// Check if the buildable is on an ele
	trace_t tr2;
	UTIL_TraceLine( m_vecBuildAirOrigin, m_vecBuildAirOrigin - Vector( 0, 0, 128 ), CONTENTS_MOVEABLE|CONTENTS_SOLID, m_pPlayer, COLLISION_GROUP_PLAYER, &tr2 );
	if( tr2.DidHit() && tr2.m_pEnt )
	{
		if( tr2.m_pEnt->GetMoveType() == MOVETYPE_PUSH )
		{
			m_BuildResult = BUILD_MOVEABLE;
			return true;
		}
	}

	// No geometry/objects in the way, hooray!
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Displays the reason the build failed 
//-----------------------------------------------------------------------------
void CFFBuildableInfo::GetBuildError( void )
{
	// TODO: Hook into the hint system so hints are displayed
	// and not the devmsg/warning stuff

	if( m_BuildResult != BUILD_ALLOWED )
	{
		char szError[512];
		Q_strcpy( szError, "#FF_BUILDERROR_" );

		switch( m_BuildResult ) 
		{
			case BUILD_NOPLAYER: Q_strncat(szError, "NOPLAYER", sizeof(szError)); break;
			case BUILD_NOROOM: Q_strncat(szError, "WORLDBLOCK", sizeof(szError)); break;
			case BUILD_TOOSTEEP: Q_strncat(szError, "GROUNDSTEEP", sizeof(szError)); break;
			case BUILD_TOOFAR: Q_strncat(szError, "GROUNDDISTANCE", sizeof(szError)); break;
			case BUILD_PLAYEROFFGROUND: Q_strncat( szError, "OFFGROUND", sizeof( szError ) ); break;
			case BUILD_MOVEABLE: Q_strncat( szError, "MOVEABLE", sizeof( szError ) ); break;

			default: Q_strncat(szError, "GENERIC", sizeof(szError)); break;
		}

#ifdef GAME_DLL
		ClientPrint( m_pPlayer, HUD_PRINTCENTER, szError );

		if(m_pPlayer)
		{
			Omnibot::Notify_Build_CantBuild(m_pPlayer, m_pPlayer->GetWantBuild());
		}
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sees if a buildable can be placed on the ground
//-----------------------------------------------------------------------------
BuildInfoResult_t CFFBuildableInfo::CanOrientToGround( void ) 
{
	Vector vecNormal, vecGround;
	float flTestDist = 64.0f;

	switch( m_iBuildObject )
	{
		case FF_BUILD_DISPENSER:
		{
			Vector vecRightAdjWidth = m_vecPlayerRight * FF_BUILD_DISP_HALF_WIDTH;
			Vector vecForwardAdjWidth = m_vecPlayerForward * FF_BUILD_DISP_HALF_WIDTH;

			// These are object aligned positions - to orient us to the ground correctly
			Vector vecCorners[ 4 ];
			vecCorners[ 0 ] = m_vecBuildAirOrigin + ( vecRightAdjWidth ) + ( vecForwardAdjWidth ) + Vector( 0, 0, 2.0f ); // Front right corner
			vecCorners[ 1 ] = m_vecBuildAirOrigin + ( vecRightAdjWidth ) - ( vecForwardAdjWidth ) + Vector( 0, 0, 2.0f ); // Back right corner
			vecCorners[ 2 ] = m_vecBuildAirOrigin - ( vecRightAdjWidth ) + ( vecForwardAdjWidth ) + Vector( 0, 0, 2.0f ); // Front left corner
			vecCorners[ 3 ] = m_vecBuildAirOrigin - ( vecRightAdjWidth ) - ( vecForwardAdjWidth ) + Vector( 0, 0, 2.0f ); // Back left corner

#ifdef FF_BUILD_DEBUG_VISUALIZATIONS
#ifdef GAME_DLL
#ifdef _DEBUG
			// Some visualizations
			if( !engine->IsDedicatedServer() )
			{				
				for( int j = 0; j < 4; j++ )
				{
					// Draw object aligned corners in blue
					NDebugOverlay::Line( vecCorners[ j ], vecCorners[ j ] - Vector( 0, 0, flTestDist ), 0, 0, 255, false, 10.0f );
				}
			}
#endif // _DEBUG
#endif // GAME_DLL
#endif // FF_BUILD_DEBUG_VISUALIZATIONS

			// For feet traces to come
			trace_t tr[ 4 ];

			// Loop through and do traces from each corner of the dispenser to the ground
			// The mask is normal PLAYER_SOLID minus PLAYER_CLIP as we don't want to build on those(Bug #0000185: Buildable objects can be built on clips.) 
			// HACK Changed to COLLISION_GROUP_PROJECTILE so it doesn't clip healthpacks (Bug #0000242: SG/Disp when building clips on health pack.)
			for( int i = 0; i < 4; i++ ) 
			{
				UTIL_TraceLine(vecCorners[i], vecCorners[i] - Vector(0, 0, flTestDist), CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE, m_pPlayer, COLLISION_GROUP_BUILDABLE_BUILDING, &tr[i]);				

				// Bug #0000246: Dispenser and sg overlap if built on each other
				if( !tr[i].DidHit() )
					return BUILD_TOOFAR;
				else if( tr[i].startsolid )
					return BUILD_NOROOM;
				// squeek: allow dispensers to be built on other buildables
				//else if( ( tr[i].m_pEnt->Classify() == CLASS_SENTRYGUN ) || ( tr[i].m_pEnt->Classify() == CLASS_DISPENSER ) )
				//	return BUILD_NOROOM;
			}

			// Make an X shaped vector from our 4 corner position traces and do the cross product
			ComputeRectangularPlane( tr[0].endpos, tr[1].endpos, tr[2].endpos, tr[3].endpos, vecNormal );
			vecGround = GetMiddle( &tr[0].endpos, &tr[1].endpos, &tr[2].endpos, &tr[3].endpos );
		}
		break;

		case FF_BUILD_SENTRYGUN:
		{
			// Trace out in front of us from build origin (but raised a little as building
			// on steep ramps will auto rotate us cause the forward trace is hitting the ramp)
			trace_t tr_fwd;
			UTIL_TraceLine( m_vecBuildAirOrigin, 
				m_vecBuildAirOrigin + ( m_vecPlayerForward * 96.0f ) + Vector( 0, 0, 32.0f ), 
				MASK_SHOT, NULL, COLLISION_GROUP_NONE, &tr_fwd );

			// If we hit a wall then we want to face towards us instead
			if( tr_fwd.DidHit() ) 
			{				
				m_vecPlayerForward *= -1;
				// Redo the right vector (was making the normal negative later on)
				m_vecPlayerRight = CrossProduct( m_vecPlayerForward, Vector( 0, 0, 1 ) );
				VectorAngles( m_vecPlayerForward, m_angBuildAirAngles );
			}

			Vector vecRightAdjWidth = m_vecPlayerRight * 4.5f;//18.0f;
			Vector vecForwardAdjWidth = m_vecPlayerForward * 2.0f;//8.0f;

			// Set up these vectors to reflect the position above the ground at each
			// of the feet of the sg where we want to start traces from
			Vector vecFeet[ 3 ];
			vecFeet[ 0 ] = m_vecBuildAirOrigin + ( m_vecPlayerForward * 5.0f );//20.0f );
			vecFeet[ 1 ] = m_vecBuildAirOrigin - ( vecRightAdjWidth ) - ( vecForwardAdjWidth );
			vecFeet[ 2 ] = m_vecBuildAirOrigin + ( vecRightAdjWidth ) - ( vecForwardAdjWidth );

#ifdef FF_BUILD_DEBUG_VISUALIZATIONS
#ifdef GAME_DLL
#ifdef _DEBUG
			// Some visualizations
			if( !engine->IsDedicatedServer() )
			{
				// Trace down from the feet in the best colors in the world
				for( int j = 0; j < 3; j++ )
				{
					NDebugOverlay::Line( vecFeet[ j ], vecFeet[ j ] - Vector( 0, 0, flTestDist ), ( ( ( j == 0 ) || ( j == 1 ) ) ? 255 : 0 ), ( ( j == 1 ) ? 255 : 0 ), ( ( ( j == 1 ) || ( j == 2 ) ) ? 255 : 0 ), false, 10.0f );
				}
			}
#endif // _DEBUG
#endif // GAME_DLL
#endif // FF_BUILD_DEBUG_VISUALIZATIONS

			// For feet traces to come
			trace_t tr[3];

			// Loop through and do traces from each corner of the sentry to the ground
			// The mask is normal PLAYER_SOLID minus PLAYER_CLIP and CONTENTS_MOVEABLE as we don't want to build on those(Bug #0000185: Buildable objects can be built on clips.) 
			// HACK Changed to COLLISION_GROUP_PROJECTILE so it doesn't clip healthpacks (Bug #0000242: SG/Disp when building clips on health pack.)
			for (int i = 0; i < 3; i++) 
			{
				UTIL_TraceLine(vecFeet[i], vecFeet[i] - Vector(0, 0, flTestDist), CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE, m_pPlayer, COLLISION_GROUP_BUILDABLE_BUILDING, &tr[i]);

				// Bug #0000246: Dispenser and sg overlap if built on each other
				if( !tr[i].DidHit() )
					return BUILD_TOOFAR;
				else if( tr[i].startsolid )
					return BUILD_NOROOM;
				// squeek: allow SGs to be built on other buildables
				//else if( ( tr[i].m_pEnt->Classify() == CLASS_SENTRYGUN ) || ( tr[i].m_pEnt->Classify() == CLASS_DISPENSER ) )
				//	return BUILD_NOROOM;
			}

			float flIntercept;
			ComputeTrianglePlane( tr[0].endpos, tr[1].endpos, tr[2].endpos, vecNormal, flIntercept );
			vecGround = GetMiddle( &tr[0].endpos, &tr[1].endpos, &tr[2].endpos );
		}
		break;

		// Shouldn't get here w/ the detpack
		case FF_BUILD_DETPACK: Assert( 0 ); return BUILD_ALLOWED; break;
		case FF_BUILD_MANCANNON: Assert( 0 ); return BUILD_ALLOWED; break;

		default: AssertMsg( 0, "Invalid buildable!" ); return BUILD_ERROR; break;
	}

	float flEpsilon = vecNormal.z / VectorLength( vecNormal );

	// AfterShock: comment these 2 lines for near vertical building (but this won't allow you to build on walls unfortunately :)
	if (flEpsilon < 0.90f) 
		return BUILD_TOOSTEEP;

	m_angBuildGroundAngles = OrientToVectors( vecNormal, m_vecPlayerForward );
	m_vecBuildGroundOrigin = vecGround;

	return BUILD_ALLOWED;
}