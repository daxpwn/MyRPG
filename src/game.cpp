#include "game.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "debugmap.h"

// ASSETS_DIR dolazi iz CMakeLists.txt i pokazuje na tvoj assets/ folder.
// Zato mozes da menjas cfg fajlove i mape i da ih ucitas sa R, bez builda.
static const std::string AssetsDir = ASSETS_DIR;

// ============================================================================
//  Pomocne funkcije
// ============================================================================

// Zoom zavisi od visine prozora, da u fullscreen-u ne vidis pola sveta.
// Uvek CEO broj (2x, 3x, 4x...), jer pixel art sa 2.7x izgleda iskrivljeno.
static float ZoomForWindow()
{
    return std::max(1.0f, std::floor(GetScreenHeight() / 200.0f));
}

// Ugradjeni font je najostriji na 10, 20, 30...
static int UiFont(float uiScale)
{
    return 10 * std::max(1, static_cast<int>(std::lround(uiScale * 2.0f / 3.0f)));
}

// Kamera prati tacku, ali ne sme da pokaze crno van ivica mape.
static void FollowWithCamera(Camera2D& camera, Vector2 target, const Tilemap& map, const Tileset& tileset)
{
    float mapWidth = static_cast<float>(map.width() * tileset.tileSize());
    float mapHeight = static_cast<float>(map.height() * tileset.tileSize());

    // koliko piksela sveta staje na pola ekrana (zbog zoom-a)
    float halfViewWidth = GetScreenWidth() / 2.0f / camera.zoom;
    float halfViewHeight = GetScreenHeight() / 2.0f / camera.zoom;

    // ako je mapa manja od ekrana, samo je centriraj
    if (mapWidth <= halfViewWidth * 2.0f)
        target.x = mapWidth / 2.0f;
    else
        target.x = std::clamp(target.x, halfViewWidth, mapWidth - halfViewWidth);

    if (mapHeight <= halfViewHeight * 2.0f)
        target.y = mapHeight / 2.0f;
    else
        target.y = std::clamp(target.y, halfViewHeight, mapHeight - halfViewHeight);

    // ceo piksel -> nema tankih linija izmedju tile-ova
    camera.target = Vector2{ std::round(target.x), std::round(target.y) };
    camera.offset = Vector2{ std::round(GetScreenWidth() / 2.0f), std::round(GetScreenHeight() / 2.0f) };
}

// Natpisi mape (debug mapa). Crtaju se u koordinatama EKRANA, posle EndMode2D,
// da bi slova bila ostra i iste velicine bez obzira na zoom.
static void DrawMapLabels(const Tilemap& map, const Camera2D& camera, const Tileset& tileset)
{
    int fontSize = UiFont(camera.zoom);
    float size = static_cast<float>(tileset.tileSize());

    for (const MapLabel& label : map.labels())
    {
        Vector2 screen = GetWorldToScreen2D(Vector2{ label.column * size, label.row * size }, camera);
        if (screen.x > GetScreenWidth() || screen.y > GetScreenHeight() || screen.x < -400 || screen.y < -50)
            continue; // nije na ekranu

        int x = static_cast<int>(screen.x) + 2;
        int y = static_cast<int>(screen.y) + 2;
        DrawText(label.text.c_str(), x + 1, y + 1, fontSize, BLACK); // senka
        DrawText(label.text.c_str(), x, y, fontSize, WHITE);
    }
}

// Nisan umesto strelice misa
static void DrawCrosshair(Vector2 mouse, float uiScale)
{
    float gap = 2.0f * uiScale;
    float length = 4.0f * uiScale;
    float thick = std::max(1.0f, uiScale / 2.0f);
    Color c = Fade(WHITE, 0.9f);
    DrawLineEx(Vector2{ mouse.x - gap - length, mouse.y }, Vector2{ mouse.x - gap, mouse.y }, thick, c);
    DrawLineEx(Vector2{ mouse.x + gap, mouse.y }, Vector2{ mouse.x + gap + length, mouse.y }, thick, c);
    DrawLineEx(Vector2{ mouse.x, mouse.y - gap - length }, Vector2{ mouse.x, mouse.y - gap }, thick, c);
    DrawLineEx(Vector2{ mouse.x, mouse.y + gap }, Vector2{ mouse.x, mouse.y + gap + length }, thick, c);
    DrawRectangleV(Vector2{ mouse.x - thick / 2.0f, mouse.y - thick / 2.0f }, Vector2{ thick, thick }, c);
}

// Tekst centriran po sirini ekrana
static void DrawCentered(const char* text, int y, int fontSize, Color color)
{
    DrawText(text, (GetScreenWidth() - MeasureText(text, fontSize)) / 2, y, fontSize, color);
}

// ============================================================================
//  Ucitavanje
// ============================================================================

bool Game::load()
{
    if (!m_tileset.load(AssetsDir + "tiles.cfg") ||
        !m_worldMap.load(AssetsDir + "map.txt") ||
        !m_arenaMap.load(AssetsDir + "arena.txt") ||
        !m_player.load(AssetsDir + "player.png") ||
        !m_weapons.load(AssetsDir + "weapons.cfg") ||
        !m_survival.load(AssetsDir + "survival.cfg") ||
        !m_shop.load(AssetsDir + "shop.cfg", m_weapons))
    {
        return false; // greska je ispisana u konzoli
    }
    m_ui = LoadTexture((AssetsDir + "ui.png").c_str());
    if (m_ui.id == 0)
        return false;
    m_shop.build(m_tileset);
    m_debugMap = CreateTileDebugMap(m_tileset, m_weapons, m_debugItems);
    m_camera.zoom = ZoomForWindow();
    goToMenu();
    return true;
}

void Game::unload()
{
    // sve sto je na grafickoj oslobadjamo PRE CloseWindow
    m_chunks.clear();
    if (m_ui.id != 0)
        UnloadTexture(m_ui);
    m_survival.unload();
    m_weapons.unload();
    m_player.unload();
    m_tileset.unload();
}

void Game::reloadFiles()
{
    m_tileset.load(AssetsDir + "tiles.cfg");
    m_worldMap.load(AssetsDir + "map.txt");
    m_arenaMap.load(AssetsDir + "arena.txt");
    m_weapons.load(AssetsDir + "weapons.cfg");
    m_survival.load(AssetsDir + "survival.cfg");
    m_shop.load(AssetsDir + "shop.cfg", m_weapons);
    m_shop.build(m_tileset);
    m_debugMap = CreateTileDebugMap(m_tileset, m_weapons, m_debugItems);
    // ako smo u enterijeru, ucitaj i njegovu mapu (da R osvezi i enterijere)
    if (m_inInterior && !m_interiorFile.empty())
        m_interiorMap.load(AssetsDir + m_interiorFile);
    m_chunks.clear(); // chunkovi su "peceni" sa starim tile-ovima
}

// ============================================================================
//  Modovi
// ============================================================================

void Game::useMap(Tilemap* map, DroppedItems* items)
{
    m_map = map;
    m_items = items;
    m_chunks.clear();  // druga mapa -> stari chunkovi ne vaze
    m_bullets.clear();
}

// ============================================================================
//  Enterijeri (ulazak kroz vrata u debug modu)
// ============================================================================

void Game::enterInterior(const std::string& file, int doorColumn, int doorRow)
{
    // ucitaj mapu enterijera (pise "izlaz X Y" i "start X Y")
    if (!m_interiorMap.load(AssetsDir + file))
    {
        showMessage("Ne mogu da ucitam: " + file, Color{ 255, 110, 100, 255 });
        return;
    }
    // zapamti odakle smo usli: vracamo se JEDNO POLJE ISPOD vrata (tile-poravnato),
    // da ne upadnemo opet u ista vrata i napravimo petlju ulaz/izlaz
    float tile = static_cast<float>(m_tileset.tileSize());
    m_returnMap = m_map;
    m_returnItems = m_items;
    m_returnPos = Vector2{ doorColumn * tile, (doorRow + 1) * tile };

    m_interiorFile = file;
    m_interiorItems.clear();
    useMap(&m_interiorMap, &m_interiorItems);
    m_player.setPosition(m_interiorMap.startPosition(m_tileset));
    m_inInterior = true;
}

void Game::leaveInterior()
{
    if (m_returnMap == nullptr)
        return;
    useMap(m_returnMap, m_returnItems);
    m_player.setPosition(m_returnPos); // vec je jedno polje ISPOD vrata
    m_inInterior = false;
    m_returnMap = nullptr;
    m_returnItems = nullptr;
}

// Proverava da li igrac stoji na ulazu/izlazu i prebacuje mapu.
void Game::updateInteriorDoors()
{
    int tile = m_tileset.tileSize();
    Rectangle feet = m_player.bounds();
    int column = static_cast<int>((feet.x + feet.width / 2.0f) / tile);
    int row = static_cast<int>((feet.y + feet.height / 2.0f) / tile);

    if (!m_inInterior)
    {
        for (const MapEntrance& e : m_map->entrances())
        {
            if (e.column == column && e.row == row)
            {
                enterInterior(e.target, e.column, e.row);
                return;
            }
        }
    }
    else if (m_map->hasExit())
    {
        Vector2 ex = m_map->exitTile();
        if (static_cast<int>(ex.x) == column && static_cast<int>(ex.y) == row)
            leaveInterior();
    }
}

void Game::giveItem(const char* weaponName, int count)
{
    // po imenu, ne po id-u: ako promenis redosled u weapons.cfg, ovo i dalje radi
    int id = m_weapons.idOf(weaponName);
    if (id >= 0)
        m_inventory.add(id, count, m_weapons);
}

void Game::startDebug()
{
    m_mode = GameMode::Debug;
    m_onDebugMap = false;
    m_inInterior = false;
    m_returnMap = nullptr;
    useMap(&m_worldMap, &m_worldItems);
    m_player.setPosition(m_worldMap.startPosition(m_tileset));
    m_player.resetHealth();
    m_player.resetArmor();
    m_inShop = false;

    m_inventory.clear();
    giveItem("Glock 17", 1);
    giveItem("UZI", 1);
    giveItem("Remington 870", 1);
    giveItem("SCAR", 1);
    giveItem("Noz", 1);
    giveItem("Granata", 5);
    giveItem("Heal potion", 3);
    giveItem("Minigun", 1);
    m_inventory.select(0);

    // AWP lezi na zemlji pored tebe, da probas podizanje
    m_worldItems.clear();
    int awp = m_weapons.idOf("AWP");
    Vector2 c = m_player.center();
    if (awp >= 0)
        m_worldItems.drop(awp, 1, Vector2{ c.x + 40.0f, c.y + 4.0f });
    m_debugMap = CreateTileDebugMap(m_tileset, m_weapons, m_debugItems);

    m_held.reset();
    m_screen = Screen::Playing;
    HideCursor(); // crtamo svoj nisan
}

void Game::startSurvival()
{
    m_mode = GameMode::Survival;
    m_inInterior = false;
    m_returnMap = nullptr;
    useMap(&m_arenaMap, &m_arenaItems);
    m_arenaItems.clear();
    m_player.setPosition(m_arenaMap.startPosition(m_tileset));
    m_player.resetHealth();
    m_player.resetArmor();
    m_money = m_survival.config().startMoney; // pocetni_novac iz survival.cfg
    m_inShop = false;
    m_shopItems.clear();

    // pocinjes skromno: pistolj, noz i 3 granate. Ostalo ispadne iz kostura.
    m_inventory.clear();
    giveItem("Glock 17", 1);
    giveItem("Noz", 1);
    giveItem("Granata", 3);
    m_inventory.select(0);

    m_survival.start();
    m_held.reset();
    m_shake = 0.0f;
    m_screen = Screen::Playing;
    HideCursor();
}

void Game::goToMenu()
{
    m_screen = Screen::Menu;
    m_inInterior = false;
    m_returnMap = nullptr;
    useMap(&m_worldMap, &m_worldItems); // meni u pozadini prikazuje svet
    ShowCursor();
}

// ============================================================================
//  Jedan frejm
// ============================================================================

void Game::runFrame()
{
    // Ako prozor "zastane" (npr. dok ga vuces), GetFrameTime moze biti ogroman
    // i sve bi preletelo kroz zidove. Zato ga ogranicimo.
    float dt = std::min(GetFrameTime(), 0.05f);
    m_uiScale = ZoomForWindow();
    m_time += dt;
    if (m_messageTimer > 0.0f)
        m_messageTimer -= dt;

    // F11 / Alt+Enter radi svuda
    bool altEnter = IsKeyPressed(KEY_ENTER) && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT));
    if (IsKeyPressed(KEY_F11) || altEnter)
        ToggleBorderlessWindowed();

    // ---- 1) ulaz + 2) update, zavisi od ekrana ----
    switch (m_screen)
    {
    case Screen::Menu:
        updateMenu(dt);
        break;
    case Screen::Playing:
        if (IsKeyPressed(KEY_ESCAPE))
        {
            m_screen = Screen::Paused;
            ShowCursor();
        }
        else
        {
            updatePlaying(dt);
        }
        break;
    case Screen::Paused:
        if (IsKeyPressed(KEY_ESCAPE))
        {
            m_screen = Screen::Playing;
            HideCursor();
        }
        else if (IsKeyPressed(KEY_M))
        {
            goToMenu();
        }
        break;
    case Screen::GameOver:
        if (IsKeyPressed(KEY_ENTER) && !altEnter)
            startSurvival();
        else if (IsKeyPressed(KEY_M) || IsKeyPressed(KEY_ESCAPE))
            goToMenu();
        break;
    }
    if (m_quit)
        return;

    if (m_useChunks)
        m_chunks.update(m_camera, *m_map, m_tileset); // pre BeginDrawing!

    // ---- 3) crtanje ----
    BeginDrawing();
    ClearBackground(BLACK);

    BeginMode2D(m_camera); // sve do EndMode2D je u koordinatama sveta
    drawWorld();
    EndMode2D();

    if (m_screen == Screen::Menu)
    {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.55f));
        m_menu.draw(m_uiScale);
    }
    else
    {
        drawPlayingHud();
        if (m_screen == Screen::Paused)
            drawPauseOverlay();
        else if (m_screen == Screen::GameOver)
            drawGameOverOverlay();
    }

    EndDrawing();
}

void Game::updateMenu(float dt)
{
    m_menuTime += dt;

    // kamera polako plovi preko mape u pozadini
    m_camera.zoom = m_uiScale;
    float tile = static_cast<float>(m_tileset.tileSize());
    float w = m_map->width() * tile;
    float h = m_map->height() * tile;
    Vector2 target{ w / 2.0f + std::sin(m_menuTime * 0.15f) * w * 0.3f,
                    h / 2.0f + std::cos(m_menuTime * 0.11f) * h * 0.25f };
    FollowWithCamera(m_camera, target, *m_map, m_tileset);

    switch (m_menu.update(dt, m_uiScale))
    {
    case MenuChoice::Debug:    startDebug(); break;
    case MenuChoice::Survival: startSurvival(); break;
    case MenuChoice::Quit:     m_quit = true; break;
    default: break;
    }
}

void Game::updatePlaying(float dt)
{
    bool debugMode = (m_mode == GameMode::Debug);

    // ---- tasteri ----
    if (IsKeyPressed(KEY_F1))
        m_debug = !m_debug;
    if (IsKeyPressed(KEY_F3))
        m_useChunks = !m_useChunks;
    if (IsKeyPressed(KEY_R))
        reloadFiles();

    if (debugMode && !m_inInterior && IsKeyPressed(KEY_F2))
    {
        if (!m_onDebugMap)
        {
            m_savedPosition = m_player.position();
            useMap(&m_debugMap, &m_debugItems);
            m_player.setPosition(m_debugMap.startPosition(m_tileset));
        }
        else
        {
            useMap(&m_worldMap, &m_worldItems);
            m_player.setPosition(m_savedPosition);
        }
        m_onDebugMap = !m_onDebugMap;
    }

    // inventar: 1-9, tockic, klik na hotbar
    bool mouseOnHotbar = m_inventory.handleInput(m_uiScale);
    if (m_inventory.selectedSlot() != m_lastSlot)
    {
        m_held.reset(); // novo oruzje u ruci -> bez starog trzaja/plamena
        m_lastSlot = m_inventory.selectedSlot();
    }

    // ---- igrac i kamera ----
    m_player.update(dt, *m_map, m_tileset);
    if (debugMode)
        updateInteriorDoors(); // ulazak/izlazak iz enterijera (menja mapu)
    m_camera.zoom = m_uiScale;
    FollowWithCamera(m_camera, m_player.center(), *m_map, m_tileset);

    // nisan: ugao od ramena ka misu (mis prevodimo iz ekrana u svet preko kamere)
    Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), m_camera);
    Vector2 shoulder = m_player.shoulder();
    m_aimAngle = std::atan2(mouseWorld.y - shoulder.y, mouseWorld.x - shoulder.x) * RAD2DEG;
    const WeaponInfo* weapon = m_weapons.find(m_inventory.selectedItem());

    // Q: baci izabrano mesto na zemlju, malo ispred sebe
    if (IsKeyPressed(KEY_Q) && m_inventory.selectedItem() != Inventory::Empty)
    {
        Slot slot = m_inventory.takeSelected();
        Vector2 c = m_player.center();
        float a = m_aimAngle * DEG2RAD;
        m_items->drop(slot.id, slot.count, Vector2{ c.x + std::cos(a) * 14.0f, c.y + std::sin(a) * 14.0f });
        weapon = nullptr;
    }

    // ---- napad ----
    bool pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !mouseOnHotbar;
    bool held = IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !mouseOnHotbar;
    m_held.update(dt, weapon, held);
    if (weapon != nullptr)
    {
        m_player.faceTowards(mouseWorld); // lik gleda ka nisanu
        AttackResult result = m_held.tryAttack(*weapon, pressed, held, shoulder, Player::HandDistance,
                                               m_aimAngle, mouseWorld, m_bullets);
        if (result == AttackResult::Shot || result == AttackResult::Swing)
            m_shake = std::max(m_shake, weapon->recoil * 0.6f);
        else if (result == AttackResult::Thrown)
            m_inventory.consumeSelected(); // jedna granata manje
        else if (result == AttackResult::Used)
            useItem(*weapon);
    }

    // ---- svet ----
    m_bullets.update(dt, *m_map, m_tileset);
    if (m_mode == GameMode::Survival)
    {
        if (m_inShop)
        {
            updateShop(); // talas ceka dok si u prodavnici
        }
        else
        {
            m_survival.update(dt, m_player, *m_map, m_tileset, m_bullets, *m_items, m_weapons, m_money);
            if (m_player.isDead())
            {
                m_screen = Screen::GameOver;
                ShowCursor();
            }
            else if (shopOpen() && CheckCollisionRecs(m_player.bounds(), shopDoorRect()))
            {
                enterShop();
            }
        }
    }
    m_bullets.clearEvents(); // udarci i eksplozije traju samo jedan frejm

    m_items->update(dt);
    m_items->pickUp(m_player.bounds(), m_inventory, m_weapons);

    // tresenje ekrana: kamera se pomeri za slucajan mali iznos, pa se smiruje
    m_shake = std::max(m_shake, m_bullets.takeShake());
    if (m_shake > 0.0f)
    {
        m_camera.target.x += std::round(GetRandomValue(-100, 100) / 100.0f * m_shake);
        m_camera.target.y += std::round(GetRandomValue(-100, 100) / 100.0f * m_shake);
        m_shake -= dt * 25.0f;
    }
}

// ============================================================================
//  Crtanje
// ============================================================================

// Efekat na vratima u koja moze da se udje:
//   - presijavanje: dijagonalni blesak stalno prelazi preko vrata
//   - otskrinu: kako prilazis (unutar ~2 polja), kroz pukotinu probija toplo
//     svetlo koje se siri sto si blizi
void Game::drawDoorFx(int column, int row) const
{
    float t = static_cast<float>(m_tileset.tileSize());
    float x = column * t, y = row * t;
    Vector2 dc{ x + t / 2.0f, y + t / 2.0f };

    // koliko je igrac blizu (u poljima) -> koliko su vrata "otvorena" (0..1)
    Vector2 pc = m_player.center();
    float dist = std::sqrt((pc.x - dc.x) * (pc.x - dc.x) + (pc.y - dc.y) * (pc.y - dc.y)) / t;
    float open = std::clamp((2.2f - dist) / 1.6f, 0.0f, 1.0f);

    // --- otskrinu: vrata na sarci (sarka levo, kvaka desno) se otvaraju U DESNO ---
    if (open > 0.02f)
    {
        float fm = t * 0.09f;                   // tanak dovratak uz desnu ivicu
        float rx = x + t - fm;                  // desna ivica otvora
        float openW = open * t * 0.58f;         // koliko se otvorilo (raste kad prilazis)
        float lx = rx - openW;                  // ivica vrata koja se otvara (svinguje)
        float top = y + t * 0.10f, hh = t * 0.84f;
        float ew = std::max(1.0f, t * 0.09f);
        DrawRectangleV(Vector2{ lx, top }, Vector2{ openW, hh },
                       Fade(Color{ 28, 22, 30, 255 }, 0.88f * open));                 // tamni otvor desno
        DrawRectangleV(Vector2{ lx - ew, top }, Vector2{ ew, hh },
                       Fade(Color{ 255, 205, 135, 255 }, 0.60f * open));              // osvetljena ivica vrata
        DrawRectangleV(Vector2{ rx, top }, Vector2{ std::max(1.0f, t * 0.06f), hh },
                       Fade(Color{ 0, 0, 0, 255 }, 0.30f * open));                    // senka dovratka
    }

    // --- presijavanje: dijagonalni blesak prelazi preko vrata (iseceno na polje) ---
    Vector2 s0 = GetWorldToScreen2D(Vector2{ x, y }, m_camera);
    Vector2 s1 = GetWorldToScreen2D(Vector2{ x + t, y + t }, m_camera);
    int sw = static_cast<int>(s1.x - s0.x), sh = static_cast<int>(s1.y - s0.y);
    if (sw > 0 && sh > 0)
    {
        BeginScissorMode(static_cast<int>(s0.x), static_cast<int>(s0.y), sw, sh);
        float cycle = 2.4f; // sekundi po blesku (pa pauza)
        float ph = std::fmod(m_time + column * 0.31f + row * 0.17f, cycle) / cycle;
        float sweep = ph / 0.40f; // blesak traje prvih ~40% ciklusa
        if (sweep <= 1.0f)
        {
            float gx = x - t * 0.7f + sweep * (t * 2.4f);
            float a = 0.55f * std::sin(sweep * PI); // jace na sredini prelaza
            DrawLineEx(Vector2{ gx, y - t * 0.2f }, Vector2{ gx - t, y + t * 1.2f }, 2.2f, Fade(WHITE, a));
            DrawLineEx(Vector2{ gx + t * 0.26f, y - t * 0.2f }, Vector2{ gx - t * 0.74f, y + t * 1.2f },
                       1.1f, Fade(WHITE, a * 0.65f));
        }
        EndScissorMode();
    }
}

void Game::drawWorld() const
{
    if (m_useChunks)
        m_chunks.draw();
    else
        m_map->draw(m_tileset);

    if (m_screen == Screen::Menu)
        return; // u meniju se vidi samo mapa

    m_items->draw(m_weapons);
    if (m_mode == GameMode::Survival)
    {
        if (m_inShop)
        {
            m_shop.draw(m_tileset, m_weapons, m_shopHighlight, m_time);
        }
        else
        {
            if (shopOpen())
            {
                // vrata prodavnice: svetle i "disu"
                Rectangle door = shopDoorRect();
                Vector2 c{ door.x + door.width / 2.0f, door.y + door.height / 2.0f };
                DrawCircleV(c, 11.0f + 2.0f * std::sin(m_time * 4.0f), Fade(GOLD, 0.25f));
                m_tileset.drawTile(m_tileset.idOf("vrata"), door.x, door.y);
            }
            m_survival.draw();
        }
    }

    // vrata enterijera: presijavanje + otskrinu kad pridjes (bez okruglih efekata)
    if (m_mode == GameMode::Debug && m_screen != Screen::Menu)
    {
        if (!m_inInterior)
            for (const MapEntrance& e : m_map->entrances())
                drawDoorFx(e.column, e.row);
        else if (m_map->hasExit())
            drawDoorFx(static_cast<int>(m_map->exitTile().x), static_cast<int>(m_map->exitTile().y));
    }

    // Kad nisanis nagore (vise od ~20 stepeni), oruzje je IZA lika; inace ispred.
    const WeaponInfo* weapon = m_weapons.find(m_inventory.selectedItem());
    Vector2 shoulder = m_player.shoulder();
    bool behind = std::sin(m_aimAngle * DEG2RAD) < -0.35f;
    if (weapon != nullptr && behind)
        m_held.draw(m_weapons, *weapon, shoulder, Player::HandDistance, m_aimAngle);
    m_player.draw();
    if (weapon != nullptr && !behind)
        m_held.draw(m_weapons, *weapon, shoulder, Player::HandDistance, m_aimAngle);

    m_bullets.draw();

    if (m_debug)
    {
        if (m_useChunks)
            m_chunks.drawDebug();
        m_player.drawDebug();
    }
}

void Game::drawPlayingHud() const
{
    bool debugMode = (m_mode == GameMode::Debug);
    int screenW = GetScreenWidth();

    DrawMapLabels(*m_map, m_camera, m_tileset);

    // natpisi iznad vrata enterijera
    if (m_mode == GameMode::Debug)
    {
        float t = static_cast<float>(m_tileset.tileSize());
        int font = UiFont(m_uiScale);
        auto doorLabel = [&](Vector2 tilePos, const char* text, Color color) {
            Vector2 s = GetWorldToScreen2D(Vector2{ tilePos.x * t + t / 2.0f, tilePos.y * t - 2.0f }, m_camera);
            int w = MeasureText(text, font);
            DrawText(text, static_cast<int>(s.x) - w / 2 + 1, static_cast<int>(s.y) - font + 1, font, BLACK);
            DrawText(text, static_cast<int>(s.x) - w / 2, static_cast<int>(s.y) - font, font, color);
        };
        if (!m_inInterior)
            for (const MapEntrance& e : m_map->entrances())
                doorLabel(Vector2{ (float)e.column, (float)e.row }, "UDJI", GOLD);
        else if (m_map->hasExit())
            doorLabel(m_map->exitTile(), "IZLAZ", SKYBLUE);
    }

    // gornja traka sa kontrolama
    DrawRectangle(0, 0, screenW, 48, Fade(BLACK, 0.6f));
    DrawText("WASD: kretanje   Mis: nisan   Levi klik: napad   1-9 / tockic: oruzje   Q: baci",
             10, 6, 16, RAYWHITE);
    DrawText(debugMode ? "Esc: pauza   F2: svi tile-ovi i oruzja   F1: debug   F11: fullscreen   R: ucitaj ponovo"
                       : "Esc: pauza   F1: debug   F11: fullscreen",
             10, 26, 16, RAYWHITE);

    // ime oruzja ide iznad srca i armora
    m_inventory.drawHotbar(m_weapons, m_uiScale, 22.0f * m_uiScale);
    drawHeartsAndArmor();

    int infoY = 56;
    int font = UiFont(m_uiScale);
    if (m_mode == GameMode::Survival)
    {
        if (m_inShop)
        {
            m_shop.drawLabels(m_camera, m_tileset, m_weapons, m_money, m_uiScale);
            // naslov na tamnoj traci, da se ne mesa sa imenima stvari
            DrawRectangle(0, 48, screenW, 2 * font + 26, Fade(BLACK, 0.55f));
            DrawCentered("PRODAVNICA", 54, font + 10, GOLD);
            DrawCentered("izlaz: vrata dole   -   talas ceka dok si ovde", 54 + font + 12, font, RAYWHITE);

            // "E: kupi ..." iznad srca
            if (m_shopHighlight >= 0)
            {
                Rectangle bar = m_inventory.hotbarRect(m_uiScale);
                int y = static_cast<int>(bar.y - 22.0f * m_uiScale) - font - 12;
                std::string text = m_shop.prompt(m_shopHighlight, m_money, m_weapons);
                DrawRectangle((screenW - MeasureText(text.c_str(), font)) / 2 - 8, y - 4,
                              MeasureText(text.c_str(), font) + 16, font + 8, Fade(BLACK, 0.6f));
                DrawCentered(text.c_str(), y, font, WHITE);
            }
        }
        else
        {
            m_survival.drawHud(m_uiScale);
            if (shopOpen())
            {
                // natpis iznad vrata prodavnice
                Rectangle door = shopDoorRect();
                Vector2 s = GetWorldToScreen2D(Vector2{ door.x + door.width / 2.0f, door.y - 2.0f }, m_camera);
                const char* label = "PRODAVNICA";
                int w = MeasureText(label, font);
                DrawText(label, static_cast<int>(s.x) - w / 2 + 1, static_cast<int>(s.y) - font + 1, font, BLACK);
                DrawText(label, static_cast<int>(s.x) - w / 2, static_cast<int>(s.y) - font, font, GOLD);
            }
        }
    }

    // kratka poruka (kupovina, napitak...) na sredini, iznad hotbara
    if (m_messageTimer > 0.0f)
    {
        Rectangle bar = m_inventory.hotbarRect(m_uiScale);
        int y = static_cast<int>(bar.y - 22.0f * m_uiScale) - 2 * font - 30;
        DrawCentered(m_message.c_str(), y, font, Fade(m_messageColor, std::min(1.0f, m_messageTimer)));
    }

    if (m_debug)
    {
        DrawFPS(10, infoY);
        Vector2 p = m_player.center();
        int column = static_cast<int>(p.x) / m_tileset.tileSize();
        int row = static_cast<int>(p.y) / m_tileset.tileSize();
        DrawText(TextFormat("igrac: kolona %d, red %d, tile id %d", column, row, m_map->get(column, row)),
                 10, infoY + 24, 18, YELLOW);
        DrawText(TextFormat("chunkovi: %s   ucitano %d (F3 menja)",
                            m_useChunks ? "UKLJUCENI" : "ISKLJUCENI", m_chunks.loadedCount()),
                 10, infoY + 48, 18, YELLOW);
        DrawText(TextFormat("prozor %d x %d   zoom %.0fx   mapa %d x %d   metaka: %d",
                            GetScreenWidth(), GetScreenHeight(), m_camera.zoom, m_map->width(), m_map->height(),
                            m_bullets.bulletCount()),
                 10, infoY + 72, 18, YELLOW);
    }

    if (m_screen == Screen::Playing)
        DrawCrosshair(GetMousePosition(), m_uiScale);
}

void Game::drawPauseOverlay() const
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.6f));
    int big = UiFont(m_uiScale) * 2;
    int small = UiFont(m_uiScale);
    int y = GetScreenHeight() / 3;
    DrawCentered("PAUZA", y, big, GOLD);
    DrawCentered("Esc: nastavi        M: glavni meni", y + big + 16, small, RAYWHITE);
}

void Game::drawGameOverOverlay() const
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(Color{ 70, 0, 0, 255 }, 0.65f));
    int big = UiFont(m_uiScale) * 3;
    int small = UiFont(m_uiScale);
    int y = GetScreenHeight() / 3;
    DrawCentered("PAO SI", y, big, Color{ 255, 90, 80, 255 });
    DrawCentered(TextFormat("Talas: %d      Ubijeno kostura: %d", m_survival.wave(), m_survival.kills()),
                 y + big + 16, small, RAYWHITE);
    DrawCentered("Enter: igraj ponovo        M: glavni meni", y + big + small + 40, small, GOLD);
}

// ============================================================================
//  Prodavnica
// ============================================================================

bool Game::shopOpen() const
{
    // vrata se vide samo u pauzi izmedju talasa (posle prvog talasa)
    return m_mode == GameMode::Survival && m_survival.inBreak() && m_survival.wave() > 0 &&
           m_arenaMap.hasShopDoor();
}

Rectangle Game::shopDoorRect() const
{
    float tile = static_cast<float>(m_tileset.tileSize());
    Vector2 door = m_arenaMap.shopDoor();
    return Rectangle{ door.x * tile, door.y * tile, tile, tile };
}

void Game::enterShop()
{
    m_inShop = true;
    m_shopHighlight = -1;
    useMap(&m_shop.map(), &m_shopItems);
    m_player.setPosition(m_shop.entrancePosition(m_tileset));
}

void Game::leaveShop()
{
    m_inShop = false;
    useMap(&m_arenaMap, &m_arenaItems);
    // pojavi se jedno polje ispod vrata (da odmah ne udjes nazad)
    Rectangle door = shopDoorRect();
    m_player.setPosition(Vector2{ door.x, door.y + door.height });
    m_survival.ensureBreakAtLeast(3.0f); // par sekundi da se spremis
}

void Game::updateShop()
{
    // stvar ispred koje stojis (po sredini stopala)
    Rectangle feet = m_player.bounds();
    Vector2 point{ feet.x + feet.width / 2.0f, feet.y + feet.height / 2.0f };
    m_shopHighlight = m_shop.nearestOffer(point, m_tileset);

    if (m_shopHighlight >= 0 && IsKeyPressed(KEY_E))
    {
        bool success = false;
        std::string text = m_shop.buy(m_shopHighlight, m_money, m_inventory, m_player, m_weapons, success);
        showMessage(text, success ? Color{ 120, 230, 120, 255 } : Color{ 255, 110, 100, 255 });
    }

    if (CheckCollisionRecs(feet, m_shop.exitDoor(m_tileset)))
        leaveShop();
}

// ============================================================================
//  Napitak i armor
// ============================================================================

void Game::useItem(const WeaponInfo& item)
{
    int points = static_cast<int>(item.damage); // za armor: koliko poena daje
    switch (item.type)
    {
    case WeaponType::Potion:
        if (m_player.health() >= m_player.maxHealth())
        {
            showMessage("Zivot je vec pun", Color{ 255, 200, 120, 255 });
            return;
        }
        m_player.heal(item.damage);
        m_inventory.consumeSelected();
        m_bullets.spawnHitEffect(m_player.center(), Vector2{ 0.0f, -1.0f }, Color{ 120, 255, 140, 255 }, 10);
        showMessage(TextFormat("+%.0f zivota", item.damage), Color{ 120, 230, 120, 255 });
        break;
    case WeaponType::BodyArmor:
        if (!m_player.equipBodyArmor(points))
        {
            showMessage("Vec imas pancir", Color{ 255, 200, 120, 255 });
            return;
        }
        m_inventory.consumeSelected();
        showMessage("Obukao si pancir", Color{ 180, 200, 255, 255 });
        break;
    case WeaponType::Helmet:
        if (!m_player.equipHelmet(points))
        {
            showMessage("Vec imas kacigu", Color{ 255, 200, 120, 255 });
            return;
        }
        m_inventory.consumeSelected();
        showMessage("Stavio si kacigu", Color{ 180, 200, 255, 255 });
        break;
    default:
        break;
    }
}

void Game::showMessage(const std::string& text, Color color)
{
    m_message = text;
    m_messageColor = color;
    m_messageTimer = 2.0f;
}

// ============================================================================
//  Srca, armor, novcici
// ============================================================================

void Game::drawHeartsAndArmor() const
{
    // ui.png: polja 16x16, ikonice 9x9 u gornjem levom uglu
    //   0 srce puno, 1 pola, 2 prazno, 3 armor pun, 4 pola, 5 prazan, 6 novcic
    auto drawIcon = [&](int index, float x, float y, float size) {
        Rectangle source{ index * 16.0f, 0.0f, 9.0f, 9.0f };
        DrawTexturePro(m_ui, source, Rectangle{ x, y, size, size }, Vector2{ 0, 0 }, 0.0f, WHITE);
    };

    float s = m_uiScale;
    float icon = 9.0f * s;  // ikonica uvecana kao i sve ostalo
    float step = 8.0f * s;  // preklapaju se 1 piksel, kao u Minecraft-u
    Rectangle bar = m_inventory.hotbarRect(s);
    float heartsY = bar.y - icon - 3.0f * s;
    float armorY = heartsY - icon - 1.0f * s;

    // srca: svako srce = 10 zivota, pola srca = 5
    int hearts = static_cast<int>(m_player.maxHealth() / 10.0f);
    int halves = static_cast<int>(std::ceil(m_player.health() / 5.0f));
    bool low = m_player.health() <= 20.0f && !m_player.isDead();
    for (int i = 0; i < hearts; i++)
    {
        int left = halves - i * 2;
        int index = left >= 2 ? 0 : (left == 1 ? 1 : 2);
        // kad je malo zivota, srca drhte
        float shake = (low && std::sin(m_time * 25.0f + i * 1.7f) > 0.6f) ? -s : 0.0f;
        drawIcon(index, bar.x + i * step, heartsY + shake, icon);
    }

    // armor: 10 mesta, 5 od pancira + 5 od kacige
    for (int i = 0; i < Player::MaxArmor; i++)
    {
        int index = i < m_player.armor() ? 3 : 5;
        drawIcon(index, bar.x + i * step, armorY, icon);
    }

    // novcici desno (samo u survival-u)
    if (m_mode == GameMode::Survival)
    {
        int font = UiFont(s);
        const char* text = TextFormat("%d", m_money);
        int w = MeasureText(text, font);
        float x = bar.x + bar.width - w;
        DrawText(text, static_cast<int>(x) + 1, static_cast<int>(heartsY + (icon - font) / 2.0f) + 1, font, BLACK);
        DrawText(text, static_cast<int>(x), static_cast<int>(heartsY + (icon - font) / 2.0f), font, GOLD);
        drawIcon(6, x - icon - 3.0f * s, heartsY, icon);
    }
}
