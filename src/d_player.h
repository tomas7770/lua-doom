//
//  Copyright (C) 1999 by
//  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
// DESCRIPTION:
//
//
//-----------------------------------------------------------------------------

#ifndef __D_PLAYER__
#define __D_PLAYER__

// The player data structure depends on a number
// of other structs: items (internal inventory),
// animation states (closely tied to the sprites
// used to represent them, unfortunately).
#include "d_items.h"
#include "p_pspr.h"

// In addition, the player is just a special
// case of the generic moving object/actor.
#include "p_mobj.h"

// Finally, for odd reasons, the player input
// is buffered within the player data struct,
// as commands per game tick.
#include "d_ticcmd.h"

#include "u_mapinfo.h"

//
// Player states.
//
typedef enum
{
  // Playing or camping.
  PST_LIVE,
  // Dead on the ground, view follows killer.
  PST_DEAD,
  // Ready to restart/respawn???
  PST_REBORN            

} playerstate_t;


//
// Player internal flags, for cheats and debug.
//
typedef enum
{
  // No clipping, walk through barriers.
  CF_NOCLIP           = 1,
  // No damage, no health loss.
  CF_GODMODE          = 2,
  // Not really a cheat, just a debug aid.
  CF_NOMOMENTUM       = 4,
  // BUDDHA cheat
  CF_BUDDHA           = 8,
  // NOTARGET cheat
  CF_NOTARGET         = 16,

  CF_MAPCOORDS        = 32,
  CF_RENDERSTATS      = 64,
  CF_SHOWFPS          = 128,
  CF_LINETARGET       = 0x00080000, // Give info on the current linetarget [Nugget]
} cheat_t;


typedef enum
{
  weapswitch_none,
  weapswitch_lowering,
  weapswitch_raising,
} weapswitch_t;

//
// Extended player object info: player_t
//
typedef struct player_s
{
  mobj_t*             mo;
  playerstate_t       playerstate;
  ticcmd_t            cmd;

  // Determine POV,
  //  including viewpoint bobbing during movement.
  // Focal origin above r.z
  fixed_t             viewz;
  // Base height above floor for viewz.
  fixed_t             viewheight;
  // Bob/squat speed.
  fixed_t             deltaviewheight;
  // bounded/scaled total momentum.
  fixed_t             bob;    

  // killough 10/98: used for realistic bobbing (i.e. not simply overall speed)
  // mo->momx and mo->momy represent true momenta experienced by player.
  // This only represents the thrust that the player applies himself.
  // This avoids anomolies with such things as Boom ice and conveyors.
  fixed_t            momx, momy;      // killough 10/98

  // This is only used between levels,
  // mo->health is used during levels.
  int                 health; 
  int                 armorpoints;
  // Armor type is 0-2.
  int                 armortype;      

  // Power ups. invinc and invis are tic counters.
  int                 powers[NUMPOWERS+4];
  boolean             cards[NUMCARDS];
  boolean             backpack;
  
  // Frags, kills of other players.
  int                 frags[MAXPLAYERS];
  weapontype_t        readyweapon;
  
  // Is wp_nochange if not changing.
  weapontype_t        pendingweapon;

  boolean             weaponowned[NUMWEAPONS];
  int                 ammo[NUMAMMO];
  int                 maxammo[NUMAMMO];

  // True if button down last tic.
  int                 attackdown;
  int                 usedown;

  // Bit flags, for cheats and debug.
  // See cheat_t, above.
  int                 cheats;         

  // Refired shots are less accurate.
  int                 refire;         

   // For intermission stats.
  int                 killcount;
  int                 itemcount;
  int                 secretcount;

  // Hint messages.
  char*               message;        
  
  // For screen flashing (red or bright).
  int                 damagecount;
  int                 bonuscount;

  // Who did damage (NULL for floors/ceilings).
  mobj_t*             attacker;
  
  // So gun flashes light up areas.
  int                 extralight;

  // Current PLAYPAL, ???
  //  can be set to REDCOLORMAP for pain, etc.
  int                 fixedcolormap;

  // Player skin colorshift,
  //  0-3 for which color to draw player.
  int                 colormap;       

  // Overlay view sprites (gun, etc).
  pspdef_t            psprites[NUMPSPRITES];

  // True if secret level has been done.
  boolean             didsecret;      

  // [AM] Previous position of viewz before think.
  //      Used to interpolate between camera positions.
  fixed_t             oldviewz;

  // [Woof!] show centered "A secret is revealed!" message
  char*               secretmessage;

  // [crispy] free look / mouse look
  int                 lookdir, oldlookdir;
  boolean             centering;
  fixed_t             slope;

  // [crispy] weapon recoil pitch
  fixed_t             recoilpitch, oldrecoilpitch;

  // [crispy] variable player view bob
  fixed_t             bob2;

  weapswitch_t switching;

} player_t;


//
// INTERMISSION
// Structure passed e.g. to WI_Start(wb)
//
typedef struct
{
  boolean     in;     // whether the player is in game
    
  // Player stats, kills, collected items etc.
  int         skills;
  int         sitems;
  int         ssecret;
  int         stime; 
  int         frags[4];
  int         score;  // current score on entry, modified on return
  
} wbplayerstruct_t;

typedef struct
{
  int         epsd;   // episode # (0-2)

  // if true, splash the secret level
  boolean     didsecret;
    
  // previous and next levels, origin 0
  int         last;
  int         next;   
  int         nextep;	// for when MAPINFO progression crosses into another episode.
  mapentry_t* lastmapinfo;
  mapentry_t* nextmapinfo;
    
  int         maxkills;
  int         maxitems;
  int         maxsecret;
  int         maxfrags;

  // the par time
  int         partime;
    
  // index of this player in game
  int         pnum;   

  wbplayerstruct_t    plyr[MAXPLAYERS];

  // [FG] total time for all completed levels
  int totaltimes;

} wbstartstruct_t;


#endif

//----------------------------------------------------------------------------
//
// $Log: d_player.h,v $
// Revision 1.3  1998/05/04  21:34:15  thldrmn
// commenting and reformatting
//
// Revision 1.2  1998/01/26  19:26:31  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:07  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
