#include "debugmap.h"

#include <algorithm>
#include <string>

Tilemap CreateTileDebugMap(const Tileset& tileset, const WeaponSet& weapons, DroppedItems& items)
{
    const int margin = 2;

    // --- raspored prvog dela (oruzja na podu) ---
    const int weaponsPerRow = 8;
    const int weaponSpacingX = 5; // mesta za natpis "Remington 870"
    const int weaponSpacingY = 3;
    int weaponCount = weapons.count();
    int weaponRows = (weaponCount + weaponsPerRow - 1) / weaponsPerRow;

    // --- raspored gornjeg dela (tiles.cfg) ---
    const int cfgPerRow = 10;  // koliko tile-ova u jednom redu
    const int cfgSpacingX = 4; // razmak u tile-ovima (mesta za natpis "12 sanduk")
    const int cfgSpacingY = 3; // tile, natpis, prazan red
    int cfgCount = tileset.idCount();
    int cfgRows = (cfgCount + cfgPerRow - 1) / cfgPerRow;

    // --- raspored donjeg dela (ceo atlas) ---
    const int atlasSpacing = 2; // svako polje atlasa, pa prazan red/kolona za natpis
    int atlasColumns = tileset.atlasColumns();
    int atlasRows = tileset.atlasRows();

    int weaponsTop = margin + 1; // +1 za naslov
    int cfgTop = weaponsTop + weaponRows * weaponSpacingY + 2;
    int atlasTop = cfgTop + cfgRows * cfgSpacingY + 2;

    int width = std::max({ margin * 2 + cfgPerRow * cfgSpacingX,
                           margin * 2 + atlasColumns * atlasSpacing,
                           margin * 2 + weaponsPerRow * weaponSpacingX });
    int height = atlasTop + atlasRows * atlasSpacing + margin;

    // Pod: tile 0 iz tiles.cfg (kod tebe trava). Ako ne postoji, prazno.
    int floorId = (tileset.find(0) != nullptr) ? 0 : Tileset::EmptyId;

    Tilemap map;
    map.create(width, height, floorId);
    map.setStart(1, 1);

    // --- oruzja: svako lezi na podu, prodji preko da ga pokupis ---
    items.clear();
    float tilePixels = static_cast<float>(tileset.tileSize());
    map.addLabel(margin, weaponsTop - 1, "oruzja   (prodji preko = pokupi, Q = baci)");
    for (int id = 0; id < weaponCount; id++)
    {
        const WeaponInfo* weapon = weapons.find(id);
        if (weapon == nullptr)
            continue;

        int column = margin + (id % weaponsPerRow) * weaponSpacingX;
        int row = weaponsTop + (id / weaponsPerRow) * weaponSpacingY;
        // stvar na zemlji se pamti po centru, u pikselima sveta
        int count = weapon->maxStack; // granate: puna gomila
        items.drop(id, count, Vector2{ column * tilePixels + tilePixels / 2.0f, row * tilePixels + tilePixels / 2.0f });
        map.addLabel(column, row + 1, weapon->name);
    }

    // --- tiles.cfg ---
    map.addLabel(margin, cfgTop - 1, "tiles.cfg   (id ime)");
    for (int id = 0; id < cfgCount; id++)
    {
        const TileInfo* tile = tileset.find(id);
        if (tile == nullptr)
            continue; // rupa u numeraciji

        int column = margin + (id % cfgPerRow) * cfgSpacingX;
        int row = cfgTop + (id / cfgPerRow) * cfgSpacingY;
        map.set(column, row, id);
        map.addLabel(column, row + 1, std::to_string(id) + " " + tile->name + (tile->solid ? " [S]" : ""));
    }

    // --- ceo atlas ---
    map.addLabel(margin, atlasTop - 1, "ceo atlas   (kolona,red)   -> prepisi u tiles.cfg");
    for (int r = 0; r < atlasRows; r++)
    {
        for (int c = 0; c < atlasColumns; c++)
        {
            int column = margin + c * atlasSpacing;
            int row = atlasTop + r * atlasSpacing;
            map.set(column, row, tileset.atlasId(c, r));
            map.addLabel(column, row + 1, std::to_string(c) + "," + std::to_string(r));
        }
    }

    return map;
}
