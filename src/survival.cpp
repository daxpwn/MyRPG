#include "survival.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <queue>
#include <sstream>

static float RandomFloat(float min, float max)
{
    return min + (max - min) * (GetRandomValue(0, 10000) / 10000.0f);
}

static Vector2 Normalize(Vector2 v)
{
    float length = std::sqrt(v.x * v.x + v.y * v.y);
    if (length < 0.0001f)
        return Vector2{ 0.0f, 0.0f };
    return Vector2{ v.x / length, v.y / length };
}

// ============================================================================
//  Ucitavanje survival.cfg
// ============================================================================

bool Survival::load(const std::string& configPath)
{
    std::ifstream file(configPath);
    if (!file)
    {
        TraceLog(LOG_ERROR, "Ne mogu da otvorim %s", configPath.c_str());
        return false;
    }

    SurvivalConfig c; // pocinje sa podrazumevanim vrednostima
    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;
        std::istringstream in(line);
        std::string key;
        if (!(in >> key))
            continue;

        if (key == "atlas")
        {
            in >> c.atlas;
            continue;
        }
        if (key == "neprijatelj")
        {
            // neprijatelj ime  dole gore desno desno2 levo (po 2 broja)  zivot brzina steta od_talasa tezina
            EnemyType type;
            in >> type.name;
            for (int f = 0; f < 5; f++)
                in >> type.frames[f][0] >> type.frames[f][1];
            in >> type.healthMul >> type.speedMul >> type.damageMul >> type.fromWave >> type.weight;
            if (!in)
            {
                TraceLog(LOG_WARNING, "%s: ne razumem \"%s\"", configPath.c_str(), line.c_str());
                continue;
            }
            type.mirrorLeft = (type.frames[FrameLeft][0] < 0);
            c.types.push_back(type);
            continue;
        }

        // svaki drugi red je: kljuc broj
        float a = 0.0f;
        in >> a;

        if (key == "velicina") c.frameSize = static_cast<int>(a);
        else if (key == "broj_prvi") c.firstCount = a;
        else if (key == "broj_rast") c.countGrowth = a;
        else if (key == "zivot_prvi") c.firstHealth = a;
        else if (key == "zivot_rast") c.healthGrowth = a;
        else if (key == "brzina_prva") c.firstSpeed = a;
        else if (key == "brzina_rast") c.speedGrowth = a;
        else if (key == "brzina_max") c.maxSpeed = a;
        else if (key == "steta") c.damage = a;
        else if (key == "napad_pauza") c.attackPause = a;
        else if (key == "razmak_pojave") c.spawnInterval = a;
        else if (key == "razmak_min") c.minSpawnInterval = a;
        else if (key == "pauza_talasa") c.waveBreak = a;
        else if (key == "sansa_oruzja") c.dropChance = a;
        else if (key == "zivot_posle_talasa") c.healAfterWave = a;
        else if (key == "max_istovremeno") c.maxAlive = static_cast<int>(a);
        else if (key == "novcici_min") c.coinsMin = static_cast<int>(a);
        else if (key == "novcici_max") c.coinsMax = static_cast<int>(a);
        else if (key == "pocetni_novac") c.startMoney = static_cast<int>(a);
        else TraceLog(LOG_WARNING, "%s: nepoznat kljuc \"%s\"", configPath.c_str(), key.c_str());
    }

    if (c.types.empty())
    {
        // bez ijednog "neprijatelj" reda: bar obican kostur
        EnemyType skeleton;
        skeleton.name = "kostur";
        int frames[5][2] = { { 4, 2 }, { 5, 2 }, { 6, 2 }, { 6, 2 }, { 7, 2 } };
        for (int f = 0; f < 5; f++)
        {
            skeleton.frames[f][0] = frames[f][0];
            skeleton.frames[f][1] = frames[f][1];
        }
        c.types.push_back(skeleton);
    }

    std::string folder = std::filesystem::path(configPath).parent_path().string();
    Texture2D atlas = LoadTexture((folder + "/" + c.atlas).c_str());
    if (atlas.id == 0)
    {
        TraceLog(LOG_ERROR, "Ne mogu da ucitam %s", c.atlas.c_str());
        return false;
    }

    unload();
    m_atlas = atlas;
    m_config = c;
    return true;
}

void Survival::unload()
{
    if (m_atlas.id != 0)
        UnloadTexture(m_atlas);
    m_atlas = Texture2D{};
}

// ============================================================================
//  Talasi
// ============================================================================

void Survival::start()
{
    m_enemies.clear();
    m_coins.clear();
    m_wave = 0;
    m_kills = 0;
    m_toSpawn = 0;
    m_inBreak = true;
    m_breakTimer = 2.5f; // kratko "pripremi se" pre prvog talasa
    m_bannerTimer = 2.5f; // "PROTIVNICI DOLAZE"
    m_fieldColumn = -1;
    m_fieldRow = -1;
}

void Survival::startWave(int number)
{
    const SurvivalConfig& c = m_config;
    m_wave = number;
    m_inBreak = false;
    // svaki talas: vise kostura, vise zivota, brzi (ali ne preko maksimuma)
    m_toSpawn = static_cast<int>(c.firstCount + c.countGrowth * (number - 1));
    m_waveHealth = c.firstHealth * (1.0f + c.healthGrowth * (number - 1));
    m_waveSpeed = std::min(c.maxSpeed, c.firstSpeed + c.speedGrowth * (number - 1));
    m_spawnTimer = 0.5f;
    m_bannerTimer = 2.5f;
}

float Survival::spawnIntervalForWave() const
{
    // svaki talas kosturi izlaze ~12% cesce
    float interval = m_config.spawnInterval * std::pow(0.88f, static_cast<float>(m_wave - 1));
    return std::max(m_config.minSpawnInterval, interval);
}

void Survival::spawnEnemy(const Tilemap& map, const Tileset& tileset, Vector2 playerCenter)
{
    float tile = static_cast<float>(tileset.tileSize());
    Vector2 spot{ -1.0f, -1.0f };

    const std::vector<Vector2>& spawns = map.spawnPoints();
    if (!spawns.empty())
    {
        spot = spawns[GetRandomValue(0, static_cast<int>(spawns.size()) - 1)];
    }
    else
    {
        // mapa nema spawn tacke: nadji slobodno polje daleko od igraca
        for (int attempt = 0; attempt < 100; attempt++)
        {
            int column = GetRandomValue(0, map.width() - 1);
            int row = GetRandomValue(0, map.height() - 1);
            float dx = column * tile - playerCenter.x, dy = row * tile - playerCenter.y;
            if (walkable(column, row, map, tileset) && dx * dx + dy * dy > (8 * tile) * (8 * tile))
            {
                spot = Vector2{ static_cast<float>(column), static_cast<float>(row) };
                break;
            }
        }
        if (spot.x < 0.0f)
            return;
    }

    Enemy e;
    e.type = chooseType();
    const EnemyType& type = m_config.types[e.type];
    e.position = Vector2{ spot.x * tile + RandomFloat(-3.0f, 3.0f), spot.y * tile + RandomFloat(-3.0f, 3.0f) };
    e.health = e.maxHealth = m_waveHealth * type.healthMul;
    // malo razliciti, da ne idu u koloni
    e.speed = m_waveSpeed * type.speedMul * RandomFloat(0.85f, 1.15f);
    e.damage = m_config.damage * type.damageMul;
    e.attackTimer = 0.5f;
    m_enemies.push_back(e);
}

int Survival::chooseType() const
{
    // "rulet": saberi tezine svih tipova koji smeju u ovom talasu,
    // izvuci slucajan broj do tog zbira i vidi u ciji deo je pao
    float total = 0.0f;
    for (const EnemyType& t : m_config.types)
        if (t.fromWave <= m_wave)
            total += t.weight;
    float pick = RandomFloat(0.0f, total);
    for (int i = 0; i < static_cast<int>(m_config.types.size()); i++)
    {
        const EnemyType& t = m_config.types[i];
        if (t.fromWave > m_wave)
            continue;
        pick -= t.weight;
        if (pick <= 0.0f)
            return i;
    }
    return 0;
}

// ============================================================================
//  Flow field (BFS od igraca)
// ============================================================================

bool Survival::walkable(int column, int row, const Tilemap& map, const Tileset& tileset) const
{
    int id = map.get(column, row);
    return id != Tileset::EmptyId && !tileset.isSolid(id);
}

int Survival::distanceAt(int column, int row) const
{
    if (column < 0 || row < 0 || column >= m_fieldWidth)
        return -1;
    size_t index = static_cast<size_t>(row) * m_fieldWidth + column;
    if (index >= m_distance.size())
        return -1;
    return m_distance[index];
}

void Survival::updateFlowField(const Tilemap& map, const Tileset& tileset, int column, int row)
{
    m_fieldWidth = map.width();
    m_fieldColumn = column;
    m_fieldRow = row;
    m_distance.assign(static_cast<size_t>(map.width()) * map.height(), -1);
    if (!walkable(column, row, map, tileset))
        return;

    // BFS: krecemo od igraca i sirimo se polje po polje.
    // Svako polje dobije broj koraka do igraca.
    std::queue<std::pair<int, int>> queue;
    m_distance[static_cast<size_t>(row) * m_fieldWidth + column] = 0;
    queue.push({ column, row });

    const int dx[4] = { 1, -1, 0, 0 };
    const int dy[4] = { 0, 0, 1, -1 };
    while (!queue.empty())
    {
        auto [c, r] = queue.front();
        queue.pop();
        int next = distanceAt(c, r) + 1;
        for (int i = 0; i < 4; i++)
        {
            int nc = c + dx[i], nr = r + dy[i];
            if (walkable(nc, nr, map, tileset) && distanceAt(nc, nr) == -1)
            {
                m_distance[static_cast<size_t>(nr) * m_fieldWidth + nc] = next;
                queue.push({ nc, nr });
            }
        }
    }
}

Vector2 Survival::steer(const Enemy& enemy, Vector2 playerCenter, const Tilemap& map, const Tileset& tileset) const
{
    float tile = static_cast<float>(tileset.tileSize());
    Vector2 here = feetCenter(enemy);
    Vector2 c = center(enemy);
    Vector2 toPlayer = Normalize(Vector2{ playerCenter.x - c.x, playerCenter.y - c.y });

    int column = static_cast<int>(std::floor(here.x / tile));
    int row = static_cast<int>(std::floor(here.y / tile));
    int distance = distanceAt(column, row);

    // blizu igraca (ili van mreze): idi pravo na njega
    if (distance < 0 || distance <= 1)
        return toPlayer;

    // inace: medju 8 suseda nadji onaj koji je najblizi igracu
    int best = distance;
    Vector2 target{};
    bool found = false;
    for (int dy = -1; dy <= 1; dy++)
    {
        for (int dx = -1; dx <= 1; dx++)
        {
            if (dx == 0 && dy == 0)
                continue;
            int d = distanceAt(column + dx, row + dy);
            if (d < 0 || d >= best)
                continue;
            // dijagonalno samo ako oba "ugla" nisu zid (da ne sece cosak stuba)
            if (dx != 0 && dy != 0 &&
                (!walkable(column + dx, row, map, tileset) || !walkable(column, row + dy, map, tileset)))
                continue;
            best = d;
            target = Vector2{ (column + dx) * tile + tile / 2.0f, (row + dy) * tile + tile / 2.0f };
            found = true;
        }
    }
    if (!found)
        return toPlayer;
    return Normalize(Vector2{ target.x - here.x, target.y - here.y });
}

// ============================================================================
//  Kretanje
// ============================================================================

Rectangle Survival::feet(Vector2 p) { return Rectangle{ p.x + 4.0f, p.y + 11.0f, 8.0f, 5.0f }; }
Rectangle Survival::body(Vector2 p) { return Rectangle{ p.x + 3.0f, p.y + 2.0f, 10.0f, 14.0f }; }
Vector2 Survival::center(const Enemy& e) { return Vector2{ e.position.x + 8.0f, e.position.y + 9.0f }; }
Vector2 Survival::feetCenter(const Enemy& e) { return Vector2{ e.position.x + 8.0f, e.position.y + 13.5f }; }

void Survival::tryMove(Enemy& enemy, Vector2 delta, const Tilemap& map, const Tileset& tileset)
{
    // kao igrac: X i Y posebno, da klizi duz zida
    Vector2 next{ enemy.position.x + delta.x, enemy.position.y };
    if (!map.collides(feet(next), tileset))
        enemy.position.x = next.x;
    next = Vector2{ enemy.position.x, enemy.position.y + delta.y };
    if (!map.collides(feet(next), tileset))
        enemy.position.y = next.y;
}

void Survival::separate(const Tilemap& map, const Tileset& tileset)
{
    // Kosturi se guraju medjusobno, da ne stoje svi u istoj tacki
    const float minDistance = 9.0f;
    for (size_t i = 0; i < m_enemies.size(); i++)
    {
        for (size_t j = i + 1; j < m_enemies.size(); j++)
        {
            Vector2 a = center(m_enemies[i]);
            Vector2 b = center(m_enemies[j]);
            float dx = a.x - b.x, dy = a.y - b.y;
            float distance = std::sqrt(dx * dx + dy * dy);
            if (distance >= minDistance || distance < 0.001f)
                continue;
            float push = (minDistance - distance) / 2.0f;
            Vector2 dir{ dx / distance, dy / distance };
            tryMove(m_enemies[i], Vector2{ dir.x * push, dir.y * push }, map, tileset);
            tryMove(m_enemies[j], Vector2{ -dir.x * push, -dir.y * push }, map, tileset);
        }
    }
}

// ============================================================================
//  Update
// ============================================================================

void Survival::update(float dt, Player& player, const Tilemap& map, const Tileset& tileset,
                      BulletSystem& bullets, DroppedItems& items, const WeaponSet& weapons, int& money)
{
    float tile = static_cast<float>(tileset.tileSize());
    Vector2 playerCenter = player.center();
    if (m_bannerTimer > 0.0f)
        m_bannerTimer -= dt;

    // --- 1) talasi ---
    if (m_inBreak)
    {
        m_breakTimer -= dt;
        if (m_breakTimer <= 0.0f)
            startWave(m_wave + 1);
    }
    else if (m_toSpawn > 0)
    {
        m_spawnTimer -= dt;
        if (m_spawnTimer <= 0.0f && static_cast<int>(m_enemies.size()) < m_config.maxAlive)
        {
            spawnEnemy(map, tileset, playerCenter);
            m_toSpawn--;
            m_spawnTimer = spawnIntervalForWave();
        }
    }
    else if (m_enemies.empty())
    {
        // svi protivnici ovog talasa su mrtvi -> odmor, malo zivota, prodavnica
        m_inBreak = true;
        m_breakTimer = m_config.waveBreak;
        m_bannerTimer = 2.5f;
        player.heal(m_config.healAfterWave);
    }

    // --- 2) flow field: preracunaj kad igrac predje u drugo polje ---
    Rectangle playerFeet = player.bounds();
    int playerColumn = static_cast<int>(std::floor((playerFeet.x + playerFeet.width / 2.0f) / tile));
    int playerRow = static_cast<int>(std::floor((playerFeet.y + playerFeet.height / 2.0f) / tile));
    if (playerColumn != m_fieldColumn || playerRow != m_fieldRow || m_fieldWidth != map.width())
        updateFlowField(map, tileset, playerColumn, playerRow);

    // --- 3) svaki kostur: hodaj ka igracu, udari ga ako ga dodirujes ---
    for (Enemy& e : m_enemies)
    {
        if (e.hitFlash > 0.0f) e.hitFlash -= dt;
        if (e.attackTimer > 0.0f) e.attackTimer -= dt;

        if (e.spawnTimer > 0.0f)
        {
            e.spawnTimer -= dt; // jos izlazi iz zemlje
            continue;
        }

        Vector2 dir = steer(e, playerCenter, map, tileset);
        Vector2 move{ (dir.x * e.speed + e.knockback.x) * dt, (dir.y * e.speed + e.knockback.y) * dt };
        tryMove(e, move, map, tileset);
        float slow = std::max(0.0f, 1.0f - 8.0f * dt);
        e.knockback.x *= slow;
        e.knockback.y *= slow;
        e.walkTime += dt;

        if (std::fabs(dir.x) > std::fabs(dir.y))
            e.facing = dir.x > 0.0f ? FaceRight : FaceLeft;
        else if (dir.y != 0.0f)
            e.facing = dir.y > 0.0f ? FaceDown : FaceUp;

        if (e.attackTimer <= 0.0f && CheckCollisionRecs(body(e.position), player.body()))
        {
            player.takeDamage(e.damage, center(e));
            e.attackTimer = m_config.attackPause;
        }
    }
    separate(map, tileset);

    // --- 4) steta od metaka, noza i eksplozija ---
    applyDamage(bullets, items, weapons);

    // --- 5) novcici: privlace se igracu kad je blizu ---
    updateCoins(dt, player, map, tileset, money);
}

void Survival::updateCoins(float dt, const Player& player, const Tilemap& map, const Tileset& tileset, int& money)
{
    Vector2 target = player.center();
    for (size_t i = 0; i < m_coins.size();)
    {
        Coin& coin = m_coins[i];
        coin.age += dt;

        Vector2 to{ target.x - coin.position.x, target.y - coin.position.y };
        float distance = std::sqrt(to.x * to.x + to.y * to.y);
        if (distance < 6.0f && coin.age > 0.3f)
        {
            money++; // pokupljen
            m_coins[i] = m_coins.back();
            m_coins.pop_back();
            continue;
        }

        if (distance < 30.0f && coin.age > 0.4f)
        {
            // "magnet": leti ka igracu, sve brze sto je blize
            float pull = 160.0f + (30.0f - distance) * 8.0f;
            coin.velocity = Vector2{ to.x / distance * pull, to.y / distance * pull };
            coin.position.x += coin.velocity.x * dt;
            coin.position.y += coin.velocity.y * dt;
        }
        else
        {
            // rasprsi se kad ispadne, pa se zaustavi (trenje); ne ide kroz zid
            float slow = std::max(0.0f, 1.0f - 5.0f * dt);
            coin.velocity.x *= slow;
            coin.velocity.y *= slow;
            Vector2 next{ coin.position.x + coin.velocity.x * dt, coin.position.y + coin.velocity.y * dt };
            if (!map.collides(Rectangle{ next.x - 1.0f, next.y - 1.0f, 2.0f, 2.0f }, tileset))
                coin.position = next;
        }
        i++;
    }
}

void Survival::hurt(Enemy& enemy, float damage, Vector2 push, BulletSystem& bullets)
{
    enemy.health -= damage;
    enemy.hitFlash = 0.12f;
    enemy.knockback.x += push.x;
    enemy.knockback.y += push.y;
    bullets.spawnHitEffect(center(enemy), push, Color{ 235, 235, 225, 255 }, 3); // komadici kosti
}

void Survival::applyDamage(BulletSystem& bullets, DroppedItems& items, const WeaponSet& weapons)
{
    // metci: prvi kostur na putu metka ga zaustavi
    for (Bullet& b : bullets.bullets())
    {
        if (!b.alive)
            continue;
        for (Enemy& e : m_enemies)
        {
            if (CheckCollisionPointRec(b.position, body(e.position)))
            {
                Vector2 dir = Normalize(b.velocity);
                hurt(e, b.damage, Vector2{ dir.x * 90.0f, dir.y * 90.0f }, bullets);
                b.alive = false;
                break;
            }
        }
    }

    // noz: svi kosturi u "kriski" ispred igraca
    for (const Strike& s : bullets.strikes())
    {
        for (Enemy& e : m_enemies)
        {
            Vector2 c = center(e);
            Vector2 to{ c.x - s.origin.x, c.y - s.origin.y };
            float distance = std::sqrt(to.x * to.x + to.y * to.y);
            if (distance > s.reach + 5.0f)
                continue;
            // razlika uglova, svedena na -180..180
            float angle = std::atan2(to.y, to.x) * RAD2DEG;
            float diff = std::fmod(angle - s.angleDeg + 540.0f, 360.0f) - 180.0f;
            if (std::fabs(diff) <= s.halfArcDeg || distance < 6.0f)
            {
                Vector2 dir = Normalize(to);
                hurt(e, s.damage, Vector2{ dir.x * 150.0f, dir.y * 150.0f }, bullets);
            }
        }
    }

    // eksplozije: svi u krugu, manje steta sto si dalje od centra
    for (const Blast& blast : bullets.blasts())
    {
        for (Enemy& e : m_enemies)
        {
            Vector2 c = center(e);
            Vector2 to{ c.x - blast.center.x, c.y - blast.center.y };
            float distance = std::sqrt(to.x * to.x + to.y * to.y);
            if (distance > blast.radius + 4.0f)
                continue;
            float falloff = 1.0f - 0.5f * std::min(1.0f, distance / blast.radius);
            Vector2 dir = Normalize(to);
            hurt(e, blast.damage * falloff, Vector2{ dir.x * 220.0f, dir.y * 220.0f }, bullets);
        }
    }

    // mrtvi kosturi: raspadnu se u kosti i mozda ispuste oruzje
    for (size_t i = 0; i < m_enemies.size();)
    {
        Enemy& e = m_enemies[i];
        if (e.health > 0.0f)
        {
            i++;
            continue;
        }
        m_kills++;
        Vector2 c = center(e);
        for (int k = 0; k < 4; k++)
        {
            float a = k * 90.0f * DEG2RAD;
            bullets.spawnHitEffect(c, Vector2{ std::cos(a), std::sin(a) }, Color{ 235, 235, 225, 255 }, 4);
        }

        // novcici: rasprse se na sve strane
        int coins = GetRandomValue(m_config.coinsMin, std::max(m_config.coinsMin, m_config.coinsMax));
        for (int k = 0; k < coins; k++)
        {
            float a = RandomFloat(0.0f, 2.0f * PI);
            float speed = RandomFloat(25.0f, 60.0f);
            Coin coin;
            coin.position = feetCenter(e);
            coin.velocity = Vector2{ std::cos(a) * speed, std::sin(a) * speed };
            m_coins.push_back(coin);
        }

        if (RandomFloat(0.0f, 1.0f) < m_config.dropChance)
        {
            int id = weapons.randomId();
            const WeaponInfo* info = weapons.find(id);
            if (info != nullptr)
            {
                int count = info->maxStack > 1 ? GetRandomValue(1, 2) : 1;
                items.drop(id, count, feetCenter(e));
            }
        }

        m_enemies[i] = m_enemies.back();
        m_enemies.pop_back();
    }
}

// ============================================================================
//  Crtanje
// ============================================================================

void Survival::draw() const
{
    // novcici na podu: zlatni krug koji se "vrti" (sirina mu se menja)
    for (const Coin& coin : m_coins)
    {
        float spin = std::fabs(std::cos(coin.age * 5.0f));
        Vector2 p{ std::round(coin.position.x), std::round(coin.position.y) };
        DrawEllipse(static_cast<int>(p.x), static_cast<int>(p.y + 2), 2.5f, 1.0f, Fade(BLACK, 0.3f));
        DrawEllipse(static_cast<int>(p.x), static_cast<int>(p.y - 1), 0.6f + 2.4f * spin, 3.0f, Color{ 120, 80, 20, 255 });
        DrawEllipse(static_cast<int>(p.x), static_cast<int>(p.y - 1), 0.3f + 1.7f * spin, 2.2f, Color{ 245, 200, 60, 255 });
    }

    // Crtamo od onih "dalje" (gore na ekranu) ka "blizim" (dole),
    // da protivnik ispred prekrije onog iza njega.
    std::vector<const Enemy*> sorted;
    for (const Enemy& e : m_enemies)
        sorted.push_back(&e);
    std::sort(sorted.begin(), sorted.end(),
              [](const Enemy* a, const Enemy* b) { return a->position.y < b->position.y; });

    float size = static_cast<float>(m_config.frameSize);
    for (const Enemy* e : sorted)
    {
        const EnemyType& type = m_config.types[e->type];
        Vector2 pos{ std::round(e->position.x), std::round(e->position.y) };

        // koji frejm: dole/gore jedan, u stranu dva koja se smenjuju dok hoda
        bool flip = false;
        int frame = FrameDown;
        bool step = std::fmod(e->walkTime, 0.36f) > 0.18f;
        switch (e->facing)
        {
        case FaceDown:  frame = FrameDown; break;
        case FaceUp:    frame = FrameUp; break;
        case FaceRight: frame = step ? FrameRight2 : FrameRight; break;
        case FaceLeft:
            if (type.mirrorLeft)
            {
                frame = step ? FrameRight2 : FrameRight;
                flip = true; // isti sprite kao desno, samo okrenut
            }
            else
            {
                frame = FrameLeft;
            }
            break;
        }
        Rectangle source{ type.frames[frame][0] * size, type.frames[frame][1] * size, size, size };
        if (flip)
            source.width = -size; // negativna sirina = ogledalo

        // senka
        DrawEllipse(static_cast<int>(pos.x + 8), static_cast<int>(pos.y + 15), 5.0f, 1.8f, Fade(BLACK, 0.3f));

        if (e->spawnTimer > 0.0f)
        {
            // izlazi iz zemlje: vidi se samo gornji deo sprite-a, sve vise
            float p = 1.0f - e->spawnTimer / SpawnTime;
            float visible = std::max(1.0f, std::floor(size * p));
            Rectangle part{ source.x, source.y, source.width, visible };
            DrawTextureRec(m_atlas, part, Vector2{ pos.x, pos.y + size - visible }, Fade(WHITE, 0.4f + 0.6f * p));
            continue;
        }

        // hod: poskakuje 1 piksel
        float bob = std::round(std::fabs(std::sin(e->walkTime * 10.0f)));
        Color tint = e->hitFlash > 0.0f ? Color{ 255, 110, 110, 255 } : WHITE;
        DrawTextureRec(m_atlas, source, Vector2{ pos.x, pos.y - bob }, tint);

        // traka zivota kad je ranjen
        if (e->health < e->maxHealth)
        {
            float w = 10.0f;
            float fill = std::max(0.0f, e->health / e->maxHealth);
            DrawRectangleV(Vector2{ pos.x + 3.0f, pos.y - 2.0f }, Vector2{ w, 1.5f }, Fade(BLACK, 0.6f));
            DrawRectangleV(Vector2{ pos.x + 3.0f, pos.y - 2.0f }, Vector2{ w * fill, 1.5f }, Color{ 220, 60, 60, 255 });
        }
    }
}

static void DrawShadowText(const char* text, int x, int y, int font, Color color)
{
    DrawText(text, x + 1, y + 1, font, Fade(BLACK, 0.7f * color.a / 255.0f));
    DrawText(text, x, y, font, color);
}

void Survival::drawHud(float uiScale) const
{
    int scale = std::max(1, static_cast<int>(std::lround(uiScale * 2.0f / 3.0f)));
    int font = 10 * scale;
    int bigFont = 20 * scale;
    int screenW = GetScreenWidth();

    // gore na sredini: talas i brojevi (ili odbrojavanje u pauzi)
    int remaining = static_cast<int>(m_enemies.size()) + m_toSpawn;
    const char* top = m_wave > 0 ? TextFormat("TALAS %d", m_wave) : "PRIPREMI SE";
    int y = 56;
    DrawShadowText(top, (screenW - MeasureText(top, font + 10)) / 2, y, font + 10, GOLD);
    const char* info = nullptr;
    if (m_inBreak && m_wave > 0)
        info = TextFormat("PAUZA: sledeci talas za %.0f   -   prodavnica je otvorena!", std::ceil(m_breakTimer));
    else if (m_inBreak)
        info = TextFormat("prvi talas za %.0f", std::ceil(m_breakTimer));
    else
        info = TextFormat("protivnika: %d    ubijeno: %d", remaining, m_kills);
    DrawShadowText(info, (screenW - MeasureText(info, font)) / 2, y + font + 14, font,
                   m_inBreak ? Color{ 255, 230, 140, 255 } : RAYWHITE);

    // veliki natpis na sredini ekrana (kratko, na pocetku talasa i pauze)
    if (m_bannerTimer > 0.0f)
    {
        const char* banner = nullptr;
        const char* sub = nullptr;
        if (m_inBreak && m_wave > 0)
        {
            banner = TextFormat("TALAS %d PREZIVLJEN!", m_wave);
            sub = TextFormat("+%.0f zivota   -   udji u prodavnicu kroz vrata", m_config.healAfterWave);
        }
        else if (m_inBreak)
        {
            banner = "PROTIVNICI DOLAZE";
            sub = "pripremi se";
        }
        else
        {
            banner = TextFormat("TALAS %d", m_wave);
            sub = TextFormat("%d protivnika", remaining);
        }
        float alpha = std::min(1.0f, m_bannerTimer);
        int cy = GetScreenHeight() / 3;
        DrawShadowText(banner, (screenW - MeasureText(banner, bigFont)) / 2, cy, bigFont, Fade(GOLD, alpha));
        DrawShadowText(sub, (screenW - MeasureText(sub, font)) / 2, cy + bigFont + 8, font, Fade(RAYWHITE, alpha));
    }
}
