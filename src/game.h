#pragma once
// Game = cela igra na jednom mestu: resursi, mape, ekrani (meni, igra, pauza...)
// i modovi (debug, survival). main.cpp samo pravi prozor i zove runFrame().
//
// Tok:   Menu --(izaberi mod)--> Playing --(Esc)--> Paused --(M)--> Menu
//                                   |
//                                   +--(umres u survival-u)--> GameOver
//
// U survival-u, izmedju talasa, u areni se pojave vrata prodavnice.
// Dok si u prodavnici (m_inShop), talas ceka.

#include <string>
#include "raylib.h"
#include "chunks.h"
#include "inventory.h"
#include "menu.h"
#include "player.h"
#include "shooting.h"
#include "shop.h"
#include "survival.h"
#include "tilemap.h"
#include "tileset.h"
#include "weapons.h"

enum class Screen
{
    Menu,
    Playing,
    Paused,
    GameOver,
};

enum class GameMode
{
    Debug,
    Survival,
};

class Game
{
public:
    bool load();   // sve slike i cfg fajlovi (posle InitWindow!)
    void unload(); // pre CloseWindow
    void runFrame();
    bool wantsToQuit() const { return m_quit; }

private:
    // --- pokretanje modova ---
    void startDebug();
    void startSurvival();
    void goToMenu();
    void reloadFiles(); // R
    void giveItem(const char* weaponName, int count);

    // --- prodavnica ---
    bool shopOpen() const;          // da li se vide vrata u areni
    Rectangle shopDoorRect() const; // vrata u areni (u pikselima sveta)
    void enterShop();
    void leaveShop();
    void updateShop();

    // napitak / pancir / kaciga (levi klik dok je izabrano)
    void useItem(const WeaponInfo& item);
    void showMessage(const std::string& text, Color color);

    // --- update po ekranu ---
    void updateMenu(float dt);
    void updatePlaying(float dt);

    // --- crtanje ---
    void drawWorld() const;     // unutar BeginMode2D
    void drawDoorFx(int column, int row) const; // presijavanje + otskrinu vrata
    void drawPlayingHud() const;
    void drawPauseOverlay() const;
    void drawGameOverOverlay() const;
    void drawHeartsAndArmor() const; // srca, armor i novcici iznad hotbara

    void useMap(Tilemap* map, DroppedItems* items);

    // --- enterijeri (debug mod) ---
    void enterInterior(const std::string& file, int doorColumn, int doorRow); // udji
    void leaveInterior();                          // izadji kroz vrata u enterijeru
    void updateInteriorDoors();                    // provera gazenja po vratima

    // resursi
    Tileset m_tileset;
    WeaponSet m_weapons;
    Player m_player;
    Survival m_survival;
    MainMenu m_menu;
    Shop m_shop;
    Texture2D m_ui{};   // ui.png: srca, armor, novcic

    // mape i stvari na zemlji (svaka mapa ima svoje)
    Tilemap m_worldMap, m_debugMap, m_arenaMap, m_interiorMap;
    DroppedItems m_worldItems, m_debugItems, m_arenaItems, m_shopItems, m_interiorItems;
    Tilemap* m_map = &m_worldMap;
    DroppedItems* m_items = &m_worldItems;

    Inventory m_inventory;
    HeldWeapon m_held;
    BulletSystem m_bullets;
    ChunkManager m_chunks;
    Camera2D m_camera{};

    Screen m_screen = Screen::Menu;
    GameMode m_mode = GameMode::Debug;

    // stanje igre
    bool m_onDebugMap = false;
    bool m_inInterior = false;        // da li smo u enterijeru
    Tilemap* m_returnMap = nullptr;   // mapa iz koje smo usli u enterijer
    DroppedItems* m_returnItems = nullptr;
    Vector2 m_returnPos{};            // gde da se vratimo kad izadjemo
    std::string m_interiorFile;       // ime fajla trenutnog enterijera (za R reload)
    Vector2 m_savedPosition{};
    float m_shake = 0.0f;
    int m_lastSlot = 0;
    float m_aimAngle = 0.0f;
    float m_menuTime = 0.0f;
    float m_uiScale = 3.0f;

    int m_money = 0;           // novcici (survival)
    bool m_inShop = false;
    int m_shopHighlight = -1;  // stvar ispred koje stojis u prodavnici
    std::string m_message;     // kratka poruka ("Kupljeno: AK-47")
    Color m_messageColor = WHITE;
    float m_messageTimer = 0.0f;
    float m_time = 0.0f;       // ukupno vreme, za animacije

    bool m_debug = false;
    bool m_useChunks = true;
    bool m_quit = false;
};
