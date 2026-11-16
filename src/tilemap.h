#pragma once
// Tilemap = 2D matrica id-eva tile-ova (ucitana iz map.txt).
//
//   m_tiles[red][kolona] = id
//
//   red = y (ide na dole), kolona = x (ide na desno)

#include <string>
#include <vector>
#include "raylib.h"
#include "tileset.h"

// Tekst zakacen za polje mape (koristi ga debug mapa za natpise)
struct MapLabel
{
    int column = 0;
    int row = 0;
    std::string text;
};

// Ulaz u enterijer: polje na kome stojis vodi u drugu mapu (fajl).
//   red mape:  ulaz X Y ime_fajla.txt
struct MapEntrance
{
    int column = 0;
    int row = 0;
    std::string target; // ime fajla mape u koju se ulazi
};

class Tilemap
{
public:
    bool load(const std::string& path);

    // Pravljenje mape iz koda (umesto iz fajla)
    void create(int width, int height, int fillId); // nova mapa, sva popunjena jednim id-em
    void set(int column, int row, int id);           // van mape -> ne radi nista
    void setStart(int column, int row);
    void addLabel(int column, int row, const std::string& text);
    const std::vector<MapLabel>& labels() const { return m_labels; }

    // Nacrta celu mapu. Tile (kolona, red) ide na poziciju (kolona*16, red*16).
    void draw(const Tileset& tileset) const;

    int width() const;   // broj kolona
    int height() const;  // broj redova

    // id tile-a na (kolona, red). Van mape vraca -1 (prazno).
    int get(int column, int row) const;

    // Da li pravougaonik (u pikselima sveta) dodiruje neki solid tile?
    // Ivica mape i prazna polja (-1) se racunaju kao zid.
    bool collides(Rectangle box, const Tileset& tileset) const;

    // Pocetna pozicija igraca, u pikselima sveta
    Vector2 startPosition(const Tileset& tileset) const;

    // Mesta gde se pojavljuju neprijatelji (redovi "spawn X Y" u fajlu mape),
    // u koordinatama tile-ova (kolona, red)
    const std::vector<Vector2>& spawnPoints() const { return m_spawns; }

    // Gde su vrata prodavnice (red "prodavnica X Y"), u koordinatama tile-ova
    bool hasShopDoor() const { return m_hasShopDoor; }
    Vector2 shopDoor() const { return m_shopDoor; }

    // Ulazi u enterijere (redovi "ulaz X Y fajl"), u koordinatama tile-ova
    const std::vector<MapEntrance>& entrances() const { return m_entrances; }

    // Izlaz iz enterijera (red "izlaz X Y"): polje po kome gazis da izadjes
    bool hasExit() const { return m_hasExit; }
    Vector2 exitTile() const { return m_exit; }

private:
    std::vector<std::vector<int>> m_tiles;
    std::vector<MapLabel> m_labels;
    std::vector<Vector2> m_spawns;
    std::vector<MapEntrance> m_entrances;
    bool m_hasShopDoor = false;
    Vector2 m_shopDoor{};
    bool m_hasExit = false;
    Vector2 m_exit{};
    int m_startColumn = 1;
    int m_startRow = 1;
};
