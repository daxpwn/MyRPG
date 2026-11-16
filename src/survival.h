#pragma once
// Survival mod: protivnici (kosturi, zombiji, sluz) dolaze u talasima, svaki talas je jaci.
//
//   - Talas N: vise protivnika, vise zivota, brzi (sve brojke su u survival.cfg).
//   - Svaki tip protivnika ima svoje sprite-ove i mnozioce (zivot, brzina, steta).
//   - Pojavljuju se na "spawn" tackama mape (kapije arene).
//   - Put do igraca nalaze preko "flow field"-a: BFS od igraca kaze svakom
//     polju koliko je koraka udaljeno, a kostur uvek ide ka susedu sa manjim brojem.
//     Tako zaobilaze stubove i zidove.
//   - Kad ih ubijes, ispadnu novcici (1-3), a ponekad i slucajno oruzje.
//   - Izmedju talasa je "pauza": tada je otvorena prodavnica (to vodi Game).

#include <algorithm>
#include <string>
#include <vector>
#include "raylib.h"
#include "inventory.h"
#include "player.h"
#include "shooting.h"
#include "tilemap.h"
#include "tileset.h"
#include "weapons.h"

// Jedan tip protivnika = jedan red "neprijatelj ..." iz survival.cfg
struct EnemyType
{
    std::string name;
    int frames[5][2] = {}; // kolona/red za: dole, gore, desno, desno2, levo
    bool mirrorLeft = false; // nema levog frejma -> desni okrenut ogledalski
    float healthMul = 1.0f, speedMul = 1.0f, damageMul = 1.0f;
    int fromWave = 1;
    float weight = 1.0f;
};

// Sve brojke iz survival.cfg (ovde su vrednosti ako red fali u fajlu)
struct SurvivalConfig
{
    std::string atlas = "ufeff_tiles_v2.png";
    int frameSize = 16;
    std::vector<EnemyType> types;
    float firstCount = 5.0f, countGrowth = 3.0f;
    float firstHealth = 30.0f, healthGrowth = 0.2f;
    float firstSpeed = 28.0f, speedGrowth = 2.5f, maxSpeed = 60.0f;
    float damage = 10.0f, attackPause = 0.8f;
    float spawnInterval = 1.2f, minSpawnInterval = 0.25f;
    float waveBreak = 4.0f;
    float dropChance = 0.15f;
    int coinsMin = 1, coinsMax = 3;
    int startMoney = 0;        // koliko novcica imas na pocetku
    float healAfterWave = 25.0f;
    int maxAlive = 40;
};

class Survival
{
public:
    bool load(const std::string& configPath);
    void unload();

    // Nova igra: brise kosture, talas 0, kratka pauza pa talas 1
    void start();

    // `money` = igracevi novcici; pokupljeni novcici se dodaju na njega
    void update(float dt, Player& player, const Tilemap& map, const Tileset& tileset,
                BulletSystem& bullets, DroppedItems& items, const WeaponSet& weapons, int& money);

    void draw() const;                  // protivnici i novcici (unutar BeginMode2D)
    void drawHud(float uiScale) const;  // talas, broj kostura, natpisi (posle EndMode2D)

    int wave() const { return m_wave; }
    int kills() const { return m_kills; }

    // Pauza izmedju talasa (tada je prodavnica otvorena)
    bool inBreak() const { return m_inBreak; }
    const SurvivalConfig& config() const { return m_config; }
    // Kad izadjes iz prodavnice: daj bar par sekundi pre sledeceg talasa
    void ensureBreakAtLeast(float seconds) { m_breakTimer = std::max(m_breakTimer, seconds); }

private:
    enum Facing { FaceDown = 0, FaceUp = 1, FaceRight = 2, FaceLeft = 3 };
    enum Frame { FrameDown = 0, FrameUp = 1, FrameRight = 2, FrameRight2 = 3, FrameLeft = 4 };

    static constexpr float SpawnTime = 0.6f; // koliko traje "izlazak iz zemlje"

    struct Enemy
    {
        Vector2 position{};   // gornji levi ugao sprite-a
        Vector2 knockback{};  // brzina odbacivanja (od metka, eksplozije...)
        float health = 1.0f;
        float maxHealth = 1.0f;
        float speed = 30.0f;
        float attackTimer = 0.0f; // do sledeceg udarca
        float hitFlash = 0.0f;    // > 0 = upravo pogodjen (crveni se)
        float walkTime = 0.0f;    // za animaciju hoda
        float spawnTimer = SpawnTime;
        int facing = FaceDown;
        int type = 0;             // indeks u m_config.types
        float damage = 10.0f;
    };

    struct Coin
    {
        Vector2 position{};
        Vector2 velocity{};
        float age = 0.0f;
    };

    void startWave(int number);
    float spawnIntervalForWave() const;
    void spawnEnemy(const Tilemap& map, const Tileset& tileset, Vector2 playerCenter);
    int chooseType() const;
    void updateCoins(float dt, const Player& player, const Tilemap& map, const Tileset& tileset, int& money);

    void updateFlowField(const Tilemap& map, const Tileset& tileset, int column, int row);
    int distanceAt(int column, int row) const;
    bool walkable(int column, int row, const Tilemap& map, const Tileset& tileset) const;
    Vector2 steer(const Enemy& enemy, Vector2 playerCenter, const Tilemap& map, const Tileset& tileset) const;
    void tryMove(Enemy& enemy, Vector2 delta, const Tilemap& map, const Tileset& tileset);
    void separate(const Tilemap& map, const Tileset& tileset);

    void applyDamage(BulletSystem& bullets, DroppedItems& items, const WeaponSet& weapons);
    void hurt(Enemy& enemy, float damage, Vector2 push, BulletSystem& bullets);

    static Rectangle feet(Vector2 position);  // za zidove
    static Rectangle body(Vector2 position);  // za metke i udarce
    static Vector2 center(const Enemy& enemy);
    static Vector2 feetCenter(const Enemy& enemy);

    SurvivalConfig m_config;
    Texture2D m_atlas{};
    std::vector<Enemy> m_enemies;
    std::vector<Coin> m_coins;

    int m_wave = 0;
    int m_toSpawn = 0;          // koliko jos kostura treba da izadje u ovom talasu
    int m_kills = 0;
    float m_spawnTimer = 0.0f;
    bool m_inBreak = true;      // pauza izmedju talasa
    float m_breakTimer = 0.0f;
    float m_bannerTimer = 0.0f; // veliki natpis "TALAS 3"
    float m_waveHealth = 0.0f;
    float m_waveSpeed = 0.0f;

    // flow field: m_distance[red * sirina + kolona] = koraka do igraca (-1 = ne moze)
    std::vector<int> m_distance;
    int m_fieldWidth = 0;
    int m_fieldColumn = -1, m_fieldRow = -1; // za koje polje igraca je izracunat
};
