#pragma once
// Debug mapa: generise se iz koda i pokazuje SVE tile-ove koje imas.
//
//   Gore:   sva oruzja iz weapons.cfg leze na podu (u `items`), sa imenom
//   Sredina: svaki tile iz tiles.cfg, sa natpisom "id ime"
//   Dole:   CEO atlas, svako polje sa natpisom "kolona,red"
//           (tako lako nadjes koordinate za novi red u tiles.cfg)

#include "inventory.h"
#include "tilemap.h"
#include "tileset.h"
#include "weapons.h"

// `items` se prvo isprazni, pa se u njega stave sva oruzja.
Tilemap CreateTileDebugMap(const Tileset& tileset, const WeaponSet& weapons, DroppedItems& items);
