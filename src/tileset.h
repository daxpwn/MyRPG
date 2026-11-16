#pragma once
// Tileset = atlas slika + opis svakog tile-a (iz tiles.cfg).
// Zna da nacrta tile po id-u i da kaze da li je tile "solid".

#include <string>
#include <vector>
#include "raylib.h"

// Opis jednog tile-a, jedan red iz tiles.cfg
struct TileInfo
{
    std::string name;
    int column = 0;      // kolona u atlasu
    int row = 0;         // red u atlasu
    bool solid = false;  // true = blokira kretanje
    int baseId = -1;     // tile koji se crta ispod (-1 = nijedan)
    bool defined = false; // false = ovaj id nije naveden u cfg fajlu
};

class Tileset
{
public:
    // Id-evi od 1000 navise nisu iz tiles.cfg, vec su "sirova" polja atlasa:
    //   id = 1000 + red * brojKolonaAtlasa + kolona
    // Koristi ih debug mapa da pokaze CEO atlas. Nisu solid i nemaju podlogu.
    static constexpr int AtlasIdBase = 1000;
    static constexpr int EmptyId = -1; // -1 = prazno: nista se ne crta

    // Ucita cfg fajl i atlas sliku. Vraca false ako nesto ne valja.
    bool load(const std::string& configPath);
    void unload();

    // Nacrta tile sa datim id-em tako da mu je gornji levi ugao u (x, y).
    void drawTile(int id, float x, float y) const;

    bool isSolid(int id) const;
    int tileSize() const { return m_tileSize; }

    // Vraca pokazivac na opis tile-a iz tiles.cfg, ili nullptr ako id ne postoji.
    const TileInfo* find(int id) const;
    // id tile-a po imenu iz tiles.cfg (npr. "vrata"), ili `fallback` ako ga nema
    int idOf(const std::string& name, int fallback = EmptyId) const;
    // Najveci id iz tiles.cfg + 1 (koliko mesta ima u tabeli)
    int idCount() const { return static_cast<int>(m_tiles.size()); }

    int atlasColumns() const { return m_atlas.width / m_tileSize; }
    int atlasRows() const { return m_atlas.height / m_tileSize; }
    int atlasId(int column, int row) const { return AtlasIdBase + row * atlasColumns() + column; }

private:
    // Pravougaonik u atlasu (u pikselima) za polje (kolona, red).
    Rectangle sourceRect(int column, int row) const;

    Texture2D m_atlas{};
    int m_tileSize = 16;
    std::vector<TileInfo> m_tiles; // indeks u vektoru = id tile-a
};
