#pragma once
// Prodavnica: soba koja se pravi iz koda (kao debug mapa), sa prodavcem za pultom
// i svim stvarima iz shop.cfg poredjanim po podu.
//
//   - Pridji stvari -> dole pise "E: kupi ..." -> E kupuje (ako imas novcica i mesta).
//   - Izlaz: vrata na dnu sobe (Game proverava kad stanes na njih).

#include <string>
#include <vector>
#include "raylib.h"
#include "inventory.h"
#include "player.h"
#include "tilemap.h"
#include "tileset.h"
#include "weapons.h"

class Shop
{
public:
    bool load(const std::string& configPath, const WeaponSet& weapons);
    // Napravi sobu (mapu) za trenutnu ponudu. Zovi posle load i posle R.
    void build(const Tileset& tileset);

    Tilemap& map() { return m_map; }
    Vector2 entrancePosition(const Tileset& tileset) const; // gde se igrac pojavi kad udje
    Rectangle exitDoor(const Tileset& tileset) const;       // stani ovde = izlaz

    // Stvar ispred koje igrac stoji (ili -1)
    int nearestOffer(Vector2 point, const Tileset& tileset) const;

    // Pokusaj kupovinu. Vraca poruku za igraca (uspeh ili zasto ne moze).
    std::string buy(int offer, int& money, Inventory& inventory, Player& player,
                     const WeaponSet& weapons, bool& success) const;
    // Tekst za "E: kupi ..." kad stojis ispred stvari
    std::string prompt(int offer, int money, const WeaponSet& weapons) const;

    void draw(const Tileset& tileset, const WeaponSet& weapons, int highlighted, float time) const;
    // Imena i cene, u koordinatama ekrana (posle EndMode2D)
    void drawLabels(const Camera2D& camera, const Tileset& tileset, const WeaponSet& weapons,
                    int money, float uiScale) const;

private:
    struct Offer
    {
        int itemId = -1;
        int price = 0;
        int count = 1;
        int column = 0, row = 0; // gde stoji u sobi
    };

    std::vector<Offer> m_offers;
    Tilemap m_map;
    int m_doorColumn = 0, m_doorRow = 0;
    int m_keeperColumn = 0, m_keeperRow = 0;
};
