//
// Copyright(C) 2021 by Ryan Krafnick
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	DSDA Map Format
//

#include "lprintf.h"
#include "p_spec.h"
#include "r_state.h"
#include "w_wad.h"

#include "map_format.h"

map_format_t map_format;

typedef enum {
  door_type_none = -1,
  door_type_red,
  door_type_blue,
  door_type_yellow,
  door_type_unknown = door_type_yellow,
  door_type_multiple
} door_type_t;

int dsda_DoorType(int index) {
  int special = lines[index].special;

  if (map_format.hexen) {
    if (special == 13 || special == 83)
      return door_type_unknown;

    return door_type_none;
  }

  if (heretic && special > 34)
    return door_type_none;

  if (GenLockedBase <= special && special < GenDoorBase) {
    special -= GenLockedBase;
    special = (special & LockedKey) >> LockedKeyShift;
    if (!special || special == 7)
      return door_type_multiple;
    else
      return (special - 1) % 3;
  }

  switch (special) {
    case 26:
    case 32:
    case 99:
    case 133:
      return door_type_blue;
    case 27:
    case 34:
    case 136:
    case 137:
      return door_type_yellow;
    case 28:
    case 33:
    case 134:
    case 135:
      return door_type_red;
    default:
      return door_type_none;
  }
}

dboolean dsda_IsExitLine(int index) {
  int special = lines[index].special;

  if (map_format.hexen)
    return special == 74 ||
           special == 75;

  return special == 11  ||
         special == 52  ||
         special == 197 ||
         special == 51  ||
         special == 124 ||
         special == 198;
}

dboolean dsda_IsTeleportLine(int index) {
  int special = lines[index].special;

  if (map_format.hexen)
    return special == 70 ||
           special == 71;

  if (heretic)
    return special == 39;

  return special == 39  ||
         special == 97  ||
         special == 125 ||
         special == 126;
}

// Migrate some non-hexen data to hexen format
static void dsda_MigrateMobjInfo(void) {
  int i;

  if (hexen || !map_format.hexen) return;

  for (i = mobj_types_zero; i < num_mobj_types; ++i) {
    if (mobjinfo[i].flags & MF_COUNTKILL)
      mobjinfo[i].flags2 |= MF2_MCROSS;

    if (mobjinfo[i].flags & MF_MISSILE)
      mobjinfo[i].flags2 |= MF2_PCROSS;
  }

  if (!raven) {
    mobjinfo[MT_SKULL].flags2 |= MF2_MCROSS;
    mobjinfo[MT_PLAYER].flags2 |= MF2_WINDTHRUST;
  }
}

void dsda_ApplyMapFormat(void) {
  extern void P_SpawnCompatibleSectorSpecial(sector_t *sector, int i);
  extern void P_SpawnZDoomSectorSpecial(sector_t *sector, int i);

  extern void P_PlayerInCompatibleSector(player_t *player, sector_t *sector);
  extern void P_PlayerInZDoomSector(player_t *player, sector_t *sector);
  extern void P_PlayerInHereticSector(player_t * player, sector_t * sector);
  extern void P_PlayerInHexenSector(player_t * player, sector_t * sector);

  extern void P_SpawnCompatibleScroller(line_t *l, int i);
  extern void P_SpawnZDoomScroller(line_t *l, int i);

  extern void P_SpawnCompatibleFriction(line_t *l);
  extern void P_SpawnZDoomFriction(line_t *l);

  extern void P_SpawnCompatiblePusher(line_t *l);
  extern void P_SpawnZDoomPusher(line_t *l);

  extern void P_SpawnCompatibleExtra(line_t *l, int i);
  extern void P_SpawnZDoomExtra(line_t *l, int i);

  extern void P_CrossCompatibleSpecialLine(line_t *line, int side, mobj_t *thing, dboolean bossaction);
  extern void P_CrossZDoomSpecialLine(line_t *line, int side, mobj_t *thing, dboolean bossaction);
  extern void P_CrossHereticSpecialLine(line_t *line, int side, mobj_t *thing, dboolean bossaction);
  extern void P_CrossHexenSpecialLine(line_t *line, int side, mobj_t *thing, dboolean bossaction);

  // if (W_CheckNumForName("BEHAVIOR") >= 0) {
  //   if (!hexen)
  //     I_Error("Hexen map format is only supported in Hexen!");

  // Can't just look for BEHAVIOR lumps, because some wads have vanilla and non-vanilla maps
  // Need proper per-map format swapping
  if (false) { // in-hexen zdoom format
    map_format.zdoom = true;
    map_format.hexen = true;
    map_format.polyobjs = false;
    map_format.acs = false;
    map_format.mapinfo = false;
    map_format.sndseq = false;
    map_format.sndinfo = false;
    map_format.animdefs = false;
    map_format.doublesky = false;
    map_format.map99 = false;
    map_format.friction_mask = ZDOOM_FRICTION_MASK;
    map_format.push_mask = ZDOOM_PUSH_MASK;
    map_format.generalized_mask = ~0xff;
    map_format.init_sector_special = P_SpawnZDoomSectorSpecial;
    map_format.player_in_special_sector = P_PlayerInZDoomSector;
    map_format.spawn_scroller = P_SpawnZDoomScroller;
    map_format.spawn_friction = P_SpawnZDoomFriction;
    map_format.spawn_pusher = P_SpawnZDoomPusher;
    map_format.spawn_extra = P_SpawnZDoomExtra;
    map_format.cross_special_line = P_CrossZDoomSpecialLine;
    map_format.mapthing_size = sizeof(mapthing_t);
    map_format.maplinedef_size = sizeof(hexen_maplinedef_t);
  }
  else if (hexen) {
    map_format.zdoom = false;
    map_format.hexen = true;
    map_format.polyobjs = true;
    map_format.acs = true;
    map_format.mapinfo = true;
    map_format.sndseq = true;
    map_format.sndinfo = true;
    map_format.animdefs = true;
    map_format.doublesky = true;
    map_format.map99 = true;
    map_format.friction_mask = 0; // not used
    map_format.push_mask = 0; // not used
    map_format.init_sector_special = NULL; // not used
    map_format.generalized_mask = 0; // no generalized specials
    map_format.player_in_special_sector = P_PlayerInHexenSector;
    map_format.spawn_scroller = NULL; // not used
    map_format.spawn_friction = NULL; // not used
    map_format.spawn_pusher = NULL; // not used
    map_format.spawn_extra = NULL; // not used
    map_format.cross_special_line = P_CrossHexenSpecialLine;
    map_format.mapthing_size = sizeof(mapthing_t);
    map_format.maplinedef_size = sizeof(hexen_maplinedef_t);
  }
  else {
    map_format.zdoom = false;
    map_format.hexen = false;
    map_format.polyobjs = false;
    map_format.acs = false;
    map_format.mapinfo = false;
    map_format.sndseq = false;
    map_format.sndinfo = false;
    map_format.animdefs = false;
    map_format.doublesky = false;
    map_format.map99 = false;
    map_format.friction_mask = FRICTION_MASK;
    map_format.push_mask = PUSH_MASK;
    map_format.generalized_mask = heretic ? 0 : ~31;
    map_format.init_sector_special = P_SpawnCompatibleSectorSpecial;
    map_format.player_in_special_sector = heretic ?
                                          P_PlayerInHereticSector :
                                          P_PlayerInCompatibleSector;
    map_format.spawn_scroller = P_SpawnCompatibleScroller;
    map_format.spawn_friction = P_SpawnCompatibleFriction;
    map_format.spawn_pusher = P_SpawnCompatiblePusher;
    map_format.spawn_extra = P_SpawnCompatibleExtra;
    map_format.cross_special_line = heretic ?
                                    P_CrossHereticSpecialLine :
                                    P_CrossCompatibleSpecialLine;
    map_format.mapthing_size = sizeof(doom_mapthing_t);
    map_format.maplinedef_size = sizeof(doom_maplinedef_t);
  }

  dsda_MigrateMobjInfo();
}
