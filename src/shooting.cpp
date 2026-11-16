#include "shooting.h"

#include <algorithm>
#include <cmath>

// Pomocne funkcije za uglove i vektore
static Vector2 AngleToVector(float angleDeg)
{
    float a = angleDeg * DEG2RAD; // DEG2RAD dolazi iz raylib.h
    return Vector2{ std::cos(a), std::sin(a) };
}

static Vector2 Rotate(Vector2 v, float angleDeg)
{
    float a = angleDeg * DEG2RAD;
    float c = std::cos(a), s = std::sin(a);
    return Vector2{ v.x * c - v.y * s, v.x * s + v.y * c };
}

static float Length(Vector2 v)
{
    return std::sqrt(v.x * v.x + v.y * v.y);
}

static float RandomFloat(float min, float max)
{
    return min + (max - min) * (GetRandomValue(0, 10000) / 10000.0f);
}

static bool AimsLeft(float angleDeg)
{
    return std::cos(angleDeg * DEG2RAD) < 0.0f;
}

// ============================================================================
//  HeldWeapon
// ============================================================================

void HeldWeapon::update(float dt, const WeaponInfo* weapon, bool triggerHeld)
{
    // Minigun: dok drzis klik cevi ubrzavaju (za spinUp sekundi do pune brzine),
    // kad pustis polako staju. Faza ide brze sto je m_spin veci -> frejm cevi.
    if (weapon != nullptr && weapon->spinUp > 0.0f && triggerHeld)
        m_spin = std::min(1.0f, m_spin + dt / weapon->spinUp);
    else
        m_spin = std::max(0.0f, m_spin - dt / SpinDownTime);
    m_spinPhase += dt * SpinFrameRate * m_spin;
    if (m_spinPhase > 1000.0f)
        m_spinPhase -= 1000.0f; // da broj ne raste u beskonacnost (1000 je deljivo sa 4)

    if (m_cooldown > 0.0f) m_cooldown -= dt;
    if (m_flash > 0.0f) m_flash -= dt;
    if (m_swing > 0.0f) m_swing -= dt;
    if (m_throw > 0.0f) m_throw -= dt;
    if (m_use > 0.0f) m_use -= dt;
    if (m_recoil > 0.0f) m_recoil -= dt / RecoilTime;
    if (m_recoil < 0.0f) m_recoil = 0.0f;
}

void HeldWeapon::reset()
{
    m_recoil = 0.0f;
    m_flash = 0.0f;
    m_swing = 0.0f;
    m_throw = 0.0f;
    m_use = 0.0f;
    m_spin = 0.0f;
}

Vector2 HeldWeapon::handPosition(Vector2 shoulder, float handDistance, float angleDeg)
{
    Vector2 dir = AngleToVector(angleDeg);
    return Vector2{ std::round(shoulder.x + dir.x * handDistance), std::round(shoulder.y + dir.y * handDistance) };
}

AttackResult HeldWeapon::tryAttack(const WeaponInfo& weapon, bool triggerPressed, bool triggerHeld,
                                   Vector2 shoulder, float handDistance, float aimDeg, Vector2 aimTarget,
                                   BulletSystem& bullets)
{
    if (m_cooldown > 0.0f)
        return AttackResult::None;

    // automatsko oruzje radi dok drzis taster, ostalo samo na klik
    bool wantsToAttack = weapon.automatic ? triggerHeld : triggerPressed;
    if (!wantsToAttack)
        return AttackResult::None;
    // minigun ne puca dok se cevi ne zavrte do kraja
    if (weapon.spinUp > 0.0f && m_spin < 1.0f)
        return AttackResult::None;

    m_cooldown = 1.0f / weapon.fireRate;
    Vector2 hand = handPosition(shoulder, handDistance, aimDeg);

    switch (weapon.type)
    {
    case WeaponType::Gun:
    {
        m_recoil = 1.0f;
        m_flash = FlashTime;
        Vector2 muzzle = muzzlePosition(weapon, hand, aimDeg);
        for (int i = 0; i < weapon.pellets; i++)
        {
            // svaki metak dobije malo drugaciji ugao (rasipanje) i brzinu
            float angle = aimDeg + RandomFloat(-weapon.spread, weapon.spread);
            float speed = weapon.bulletSpeed * RandomFloat(0.92f, 1.08f);
            bullets.spawnBullet(muzzle, angle, speed, weapon.range, weapon.damage);
        }
        bullets.spawnShell(hand, aimDeg);
        return AttackResult::Shot;
    }
    case WeaponType::Melee:
    {
        m_swing = SwingTime;
        m_swingSide = -m_swingSide; // naizmenicno: odozgo pa odozdo
        m_swingCenter = aimDeg;
        Strike strike;
        strike.origin = shoulder;
        strike.angleDeg = aimDeg;
        strike.halfArcDeg = weapon.spread;
        strike.reach = weapon.range;
        strike.damage = weapon.damage;
        bullets.addStrike(strike);
        return AttackResult::Swing;
    }
    case WeaponType::Throwable:
    {
        m_throw = ThrowTime;
        bullets.spawnGrenade(hand, aimTarget, weapon.bulletSpeed, weapon.range, weapon.damage);
        return AttackResult::Thrown;
    }
    case WeaponType::Potion:
    case WeaponType::BodyArmor:
    case WeaponType::Helmet:
    {
        m_use = UseTime;
        return AttackResult::Used;
    }
    }
    return AttackResult::None;
}

Vector2 HeldWeapon::muzzlePosition(const WeaponInfo& weapon, Vector2 hand, float angleDeg)
{
    // Udaljenost od drske do cevi u sprite-u...
    // (puta skala, jer se velike puske crtaju umanjeno)
    Vector2 local{ (weapon.muzzle.x - weapon.grip.x) * weapon.scale, (weapon.muzzle.y - weapon.grip.y) * weapon.scale };
    // ...kad nisanis levo, sprite je okrenut naopako, pa i y ide na drugu stranu
    if (AimsLeft(angleDeg))
        local.y = -local.y;
    // ...okrenuto za ugao nisanjenja, i pomereno do ruke
    Vector2 rotated = Rotate(local, angleDeg);
    return Vector2{ hand.x + rotated.x, hand.y + rotated.y };
}

void HeldWeapon::draw(const WeaponSet& weapons, const WeaponInfo& weapon,
                      Vector2 shoulder, float handDistance, float aimDeg) const
{
    switch (weapon.type)
    {
    case WeaponType::Gun:       drawGun(weapons, weapon, shoulder, handDistance, aimDeg); break;
    case WeaponType::Melee:     drawKnife(weapons, weapon, shoulder, handDistance, aimDeg); break;
    case WeaponType::Throwable: drawGrenade(weapons, weapon, shoulder, handDistance, aimDeg); break;
    case WeaponType::Potion:
    case WeaponType::BodyArmor:
    case WeaponType::Helmet:    drawUsable(weapons, weapon, shoulder, handDistance, aimDeg); break;
    }
}

void HeldWeapon::drawGun(const WeaponSet& weapons, const WeaponInfo& weapon,
                         Vector2 shoulder, float handDistance, float aimDeg) const
{
    bool flip = AimsLeft(aimDeg);
    Vector2 hand = handPosition(shoulder, handDistance, aimDeg);

    // trzaj: oruzje se pomeri unazad i malo podigne cev (m_recoil^2 = brzo pa sporo vracanje)
    Vector2 dir = AngleToVector(aimDeg);
    float back = weapon.recoil * m_recoil * m_recoil;
    Vector2 drawHand{ hand.x - dir.x * back, hand.y - dir.y * back };
    float kickAngle = (flip ? 1.0f : -1.0f) * weapon.recoil * 3.0f * m_recoil * m_recoil;

    int barrelFrame = static_cast<int>(m_spinPhase); // frejm cevi (za obicne puske uvek 0)
    weapons.drawHeld(weapon, drawHand, aimDeg + kickAngle, flip, barrelFrame);

    // plamen: frejm zavisi od toga koliko je vremena proslo od pucnja
    if (m_flash > 0.0f && weapons.flashFrames() > 0)
    {
        float progress = 1.0f - m_flash / FlashTime; // 0 -> 1
        int frame = std::min(static_cast<int>(progress * weapons.flashFrames()), weapons.flashFrames() - 1);
        Vector2 muzzle = muzzlePosition(weapon, drawHand, aimDeg + kickAngle);
        weapons.drawFlash(frame, muzzle, aimDeg + kickAngle);
    }
}

void HeldWeapon::drawKnife(const WeaponSet& weapons, const WeaponInfo& weapon,
                           Vector2 shoulder, float handDistance, float aimDeg) const
{
    if (m_swing <= 0.0f)
    {
        // nema zamaha: noz se drzi kao i svako oruzje, ka nisanu
        weapons.drawHeld(weapon, handPosition(shoulder, handDistance, aimDeg), aimDeg, AimsLeft(aimDeg));
        return;
    }

    // Zamah: ugao ide od jedne ivice luka do druge.
    // ease = brzo na pocetku, sporije na kraju (izgleda "jace").
    float progress = 1.0f - m_swing / SwingTime; // 0 -> 1
    float ease = 1.0f - (1.0f - progress) * (1.0f - progress);
    float startAngle = m_swingCenter + m_swingSide * weapon.spread;
    float angle = startAngle - m_swingSide * 2.0f * weapon.spread * ease;

    // trag zamaha: beli luk koji bledi
    float radius = weapon.range * 0.85f;
    float from = std::min(startAngle, angle);
    float to = std::max(startAngle, angle);
    DrawRing(shoulder, radius - 3.0f, radius, from, to, 16, Fade(WHITE, 0.55f * (1.0f - progress)));

    // noz je malo dalje od tela dok se zamahuje (ruka se ispruzi)
    Vector2 hand = handPosition(shoulder, handDistance + 3.0f, angle);
    weapons.drawHeld(weapon, hand, angle, AimsLeft(angle));
}

void HeldWeapon::drawGrenade(const WeaponSet& weapons, const WeaponInfo& weapon,
                             Vector2 shoulder, float handDistance, float aimDeg) const
{
    if (m_throw > 0.0f)
        return; // ruka je prazna dok ne "izvadi" sledecu granatu
    // granata se drzi uspravno (bez okretanja)
    weapons.drawHeld(weapon, handPosition(shoulder, handDistance, aimDeg), 0.0f, false);
}

void HeldWeapon::drawUsable(const WeaponSet& weapons, const WeaponInfo& weapon,
                            Vector2 shoulder, float handDistance, float aimDeg) const
{
    Vector2 hand = handPosition(shoulder, handDistance, aimDeg);
    float tilt = 0.0f;
    if (m_use > 0.0f)
    {
        // podigni stvar ka glavi i nagni je (kao da pijes), pa vrati
        float p = 1.0f - m_use / UseTime;     // 0 -> 1
        float up = std::sin(p * PI);          // 0 -> 1 -> 0
        hand.x = std::round(shoulder.x + (hand.x - shoulder.x) * (1.0f - up));
        hand.y = std::round(hand.y - 6.0f * up);
        tilt = -50.0f * up;
    }
    weapons.drawHeld(weapon, hand, tilt, false);
}

// ============================================================================
//  BulletSystem
// ============================================================================

static constexpr float Gravity = 420.0f;       // za let granate (piksela/s^2)
static constexpr float FlightTime = 0.5f;      // koliko granata leti do cilja
static constexpr float ExplosionTime = 0.45f;  // koliko traje slika eksplozije

void BulletSystem::spawnBullet(Vector2 position, float angleDeg, float speed, float range, float damage)
{
    Vector2 dir = AngleToVector(angleDeg);
    Bullet b;
    b.position = position;
    b.velocity = Vector2{ dir.x * speed, dir.y * speed };
    b.distanceLeft = range;
    b.damage = damage;
    m_bullets.push_back(b);
}

void BulletSystem::spawnShell(Vector2 position, float angleDeg)
{
    // Caura izleti iz "gornje" strane oruzja (pod pravim uglom na nisan), malo unazad.
    // Kad nisanis levo oruzje je okrenuto naopako, pa je "gore" na drugoj strani.
    bool flip = AimsLeft(angleDeg);
    float up = flip ? angleDeg + 90.0f : angleDeg - 90.0f;
    float backwards = flip ? 20.0f : -20.0f;
    Vector2 dir = AngleToVector(up + backwards + RandomFloat(-20.0f, 20.0f));
    float speed = RandomFloat(40.0f, 70.0f);

    Particle p;
    p.position = position;
    p.velocity = Vector2{ dir.x * speed, dir.y * speed };
    p.life = p.maxLife = 1.2f;
    p.friction = 6.0f;
    p.size = 1.5f;
    p.color = Color{ 222, 178, 70, 255 }; // mesing
    m_particles.push_back(p);
}

void BulletSystem::spawnGrenade(Vector2 from, Vector2 target, float maxDistance, float radius, float damage)
{
    Vector2 delta{ target.x - from.x, target.y - from.y };
    float distance = Length(delta);
    Vector2 dir = distance > 0.01f ? Vector2{ delta.x / distance, delta.y / distance } : Vector2{ 1.0f, 0.0f };
    distance = std::min(distance, maxDistance);

    Grenade g;
    g.position = from;
    // brzina takva da za FlightTime sekundi stigne do cilja...
    g.velocity = Vector2{ dir.x * distance / FlightTime, dir.y * distance / FlightTime };
    // ...a vertikalna brzina takva da za isto vreme padne nazad na zemlju (luk)
    g.height = 2.0f;
    g.verticalSpeed = Gravity * FlightTime / 2.0f;
    g.radius = radius;
    g.damage = damage;
    m_grenades.push_back(g);
}

void BulletSystem::spawnImpact(Vector2 position, Vector2 bulletVelocity)
{
    // varnice odskacu nazad od zida
    Vector2 back{ -bulletVelocity.x, -bulletVelocity.y };
    spawnHitEffect(position, back, Color{ 255, 220, 120, 255 }, 3);
    spawnHitEffect(position, back, Color{ 255, 150, 40, 255 }, 3);
    // malo prasine
    Particle dust;
    dust.position = position;
    dust.life = dust.maxLife = 0.25f;
    dust.size = 3.0f;
    dust.color = Color{ 200, 200, 190, 160 };
    m_particles.push_back(dust);
}

void BulletSystem::spawnHitEffect(Vector2 position, Vector2 direction, Color color, int count)
{
    float base = std::atan2(direction.y, direction.x) * RAD2DEG;
    for (int i = 0; i < count; i++)
    {
        Vector2 dir = AngleToVector(base + RandomFloat(-60.0f, 60.0f));
        float speed = RandomFloat(30.0f, 110.0f);
        Particle p;
        p.position = position;
        p.velocity = Vector2{ dir.x * speed, dir.y * speed };
        p.life = p.maxLife = RandomFloat(0.15f, 0.4f);
        p.friction = 5.0f;
        p.size = RandomFloat(1.0f, 1.8f);
        p.color = color;
        m_particles.push_back(p);
    }
}

void BulletSystem::explode(const Grenade& g)
{
    // steta (procitace je neprijatelji u ovom frejmu)
    m_blasts.push_back(Blast{ g.position, g.radius, g.damage });
    // slika eksplozije
    m_explosions.push_back(Explosion{ g.position, g.radius, 0.0f });

    // vatra: brze narandzaste/zute cestice na sve strane
    for (int i = 0; i < 22; i++)
    {
        Vector2 dir = AngleToVector(RandomFloat(0.0f, 360.0f));
        float speed = RandomFloat(50.0f, 170.0f);
        Particle p;
        p.position = g.position;
        p.velocity = Vector2{ dir.x * speed, dir.y * speed };
        p.life = p.maxLife = RandomFloat(0.2f, 0.45f);
        p.friction = 3.5f;
        p.size = RandomFloat(1.5f, 2.5f);
        p.color = (i % 3 == 0) ? Color{ 255, 240, 170, 255 } : Color{ 255, 140, 30, 255 };
        m_particles.push_back(p);
    }
    // dim: sporo, dugo, raste
    for (int i = 0; i < 10; i++)
    {
        Vector2 dir = AngleToVector(RandomFloat(0.0f, 360.0f));
        float speed = RandomFloat(8.0f, 35.0f);
        Particle p;
        p.position = Vector2{ g.position.x + dir.x * 4.0f, g.position.y + dir.y * 4.0f };
        p.velocity = Vector2{ dir.x * speed, dir.y * speed - 10.0f }; // dim se dize
        p.life = p.maxLife = RandomFloat(0.6f, 1.1f);
        p.friction = 1.5f;
        p.size = 3.0f;
        p.grow = 7.0f;
        p.color = Color{ 80, 80, 85, 170 };
        m_particles.push_back(p);
    }
    m_shake = std::max(m_shake, 7.0f);
}

bool BulletSystem::blocked(Vector2 point, const Tilemap& map, const Tileset& tileset)
{
    float tile = static_cast<float>(tileset.tileSize());
    int column = static_cast<int>(std::floor(point.x / tile));
    int row = static_cast<int>(std::floor(point.y / tile));
    int id = map.get(column, row);
    return id == Tileset::EmptyId || tileset.isSolid(id);
}

void BulletSystem::update(float dt, const Tilemap& map, const Tileset& tileset)
{
    // --- metci ---
    for (size_t i = 0; i < m_bullets.size();)
    {
        Bullet& b = m_bullets[i];

        // Metak se pomera u nekoliko malih koraka, da brzi metak ne bi
        // "preskocio" tanak zid izmedju dva frejma.
        const int steps = 4;
        for (int s = 0; s < steps && b.alive; s++)
        {
            Vector2 step{ b.velocity.x * dt / steps, b.velocity.y * dt / steps };
            b.position.x += step.x;
            b.position.y += step.y;
            b.distanceLeft -= Length(step);

            if (blocked(b.position, map, tileset))
            {
                spawnImpact(b.position, b.velocity);
                b.alive = false;
            }
            else if (b.distanceLeft <= 0.0f)
            {
                b.alive = false; // preleteo domet
            }
        }

        // metak koji je pogodio zid, ili neprijatelja u proslom frejmu, se brise
        if (!b.alive)
        {
            m_bullets[i] = m_bullets.back();
            m_bullets.pop_back();
        }
        else
        {
            i++;
        }
    }

    // --- granate ---
    for (size_t i = 0; i < m_grenades.size();)
    {
        Grenade& g = m_grenades[i];
        g.fuse -= dt;

        // visina: gravitacija je vuce na dole -> let u luku
        if (g.height > 0.0f || g.verticalSpeed > 0.0f)
        {
            g.verticalSpeed -= Gravity * dt;
            g.height += g.verticalSpeed * dt;
            if (g.height <= 0.0f)
            {
                g.height = 0.0f;
                // udarila u zemlju: odskoci malo (ako je padala brzo) i uspori
                g.verticalSpeed = (g.verticalSpeed < -60.0f) ? -g.verticalSpeed * 0.35f : 0.0f;
                g.velocity.x *= 0.6f;
                g.velocity.y *= 0.6f;
            }
        }
        else
        {
            // kotrlja se po zemlji: trenje
            float slow = std::max(0.0f, 1.0f - 4.0f * dt);
            g.velocity.x *= slow;
            g.velocity.y *= slow;
        }

        // horizontalno kretanje; od zida se odbije (X i Y posebno, kao igrac)
        Vector2 nextX{ g.position.x + g.velocity.x * dt, g.position.y };
        if (blocked(nextX, map, tileset)) g.velocity.x = -g.velocity.x * 0.5f;
        else g.position.x = nextX.x;
        Vector2 nextY{ g.position.x, g.position.y + g.velocity.y * dt };
        if (blocked(nextY, map, tileset)) g.velocity.y = -g.velocity.y * 0.5f;
        else g.position.y = nextY.y;

        g.spin += Length(g.velocity) * dt * 12.0f;

        if (g.fuse <= 0.0f)
        {
            explode(g);
            m_grenades[i] = m_grenades.back();
            m_grenades.pop_back();
        }
        else
        {
            i++;
        }
    }

    // --- slike eksplozija ---
    for (size_t i = 0; i < m_explosions.size();)
    {
        m_explosions[i].age += dt;
        if (m_explosions[i].age >= ExplosionTime)
        {
            m_explosions[i] = m_explosions.back();
            m_explosions.pop_back();
        }
        else
        {
            i++;
        }
    }

    // --- cestice ---
    for (size_t i = 0; i < m_particles.size();)
    {
        Particle& p = m_particles[i];
        p.life -= dt;
        if (p.life <= 0.0f)
        {
            m_particles[i] = m_particles.back();
            m_particles.pop_back();
            continue;
        }
        p.position.x += p.velocity.x * dt;
        p.position.y += p.velocity.y * dt;
        p.size += p.grow * dt;
        // trenje: brzina se smanjuje svaki frejm
        float slow = std::max(0.0f, 1.0f - p.friction * dt);
        p.velocity.x *= slow;
        p.velocity.y *= slow;
        i++;
    }
}

void BulletSystem::draw() const
{
    // cestice (caure, dim, varnice)
    for (const Particle& p : m_particles)
    {
        float alpha = p.life / p.maxLife; // bledi pred kraj
        Color c = p.color;
        c.a = static_cast<unsigned char>(c.a * alpha);
        DrawRectangleV(Vector2{ p.position.x - p.size / 2.0f, p.position.y - p.size / 2.0f },
                       Vector2{ p.size, p.size }, c);
    }

    // granate: senka na zemlji + telo podignuto za visinu leta
    for (const Grenade& g : m_grenades)
    {
        DrawEllipse(static_cast<int>(g.position.x), static_cast<int>(g.position.y + 1.0f), 3.0f, 1.2f, Fade(BLACK, 0.35f));
        Vector2 body{ g.position.x, g.position.y - g.height - 2.0f };
        DrawCircleV(body, 3.0f, Color{ 27, 27, 36, 255 });
        DrawCircleV(body, 2.2f, Color{ 92, 124, 76, 255 });
        // rucica koja se vrti dok granata leti
        Vector2 lever = Rotate(Vector2{ 0.0f, -3.5f }, g.spin);
        DrawLineEx(body, Vector2{ body.x + lever.x, body.y + lever.y }, 1.0f, Color{ 150, 158, 172, 255 });
        // crveno svetlo trepce sve brze kako se blizi eksplozija
        float blink = g.fuse < 0.5f ? 0.08f : 0.2f;
        if (std::fmod(g.fuse, blink * 2.0f) < blink)
            DrawRectangleV(Vector2{ body.x - 0.5f, body.y - 0.5f }, Vector2{ 1.0f, 1.0f }, RED);
    }

    // eksplozije: bljesak, vatrena lopta, talas koji se siri
    for (const Explosion& e : m_explosions)
    {
        float p = e.age / ExplosionTime;        // 0 -> 1
        float ease = 1.0f - (1.0f - p) * (1.0f - p);
        if (p < 0.2f)
            DrawCircleV(e.center, e.radius * 0.6f, Fade(WHITE, 0.9f * (1.0f - p / 0.2f)));
        DrawCircleV(e.center, e.radius * (0.3f + 0.6f * ease), Fade(ORANGE, 0.5f * (1.0f - p)));
        DrawCircleV(e.center, e.radius * (0.15f + 0.35f * ease), Fade(YELLOW, 0.7f * (1.0f - p)));
        float ring = e.radius * ease;
        DrawRing(e.center, std::max(0.0f, ring - 2.0f), ring, 0.0f, 360.0f, 32, Fade(WHITE, 0.6f * (1.0f - p)));
    }

    // metci: kratka svetla linija ("tracer") sa repom
    for (const Bullet& b : m_bullets)
    {
        if (!b.alive)
            continue; // pogodio neprijatelja, nestaje
        float speed = Length(b.velocity);
        Vector2 dir{ b.velocity.x / speed, b.velocity.y / speed };
        Vector2 tail{ b.position.x - dir.x * 7.0f, b.position.y - dir.y * 7.0f };
        DrawLineEx(tail, b.position, 1.5f, Color{ 255, 200, 80, 140 });
        Vector2 head{ b.position.x - dir.x * 2.0f, b.position.y - dir.y * 2.0f };
        DrawLineEx(head, b.position, 1.5f, Color{ 255, 250, 210, 255 });
    }
}

void BulletSystem::clearEvents()
{
    m_strikes.clear();
    m_blasts.clear();
}

float BulletSystem::takeShake()
{
    float shake = m_shake;
    m_shake = 0.0f;
    return shake;
}

void BulletSystem::clear()
{
    m_bullets.clear();
    m_particles.clear();
    m_grenades.clear();
    m_explosions.clear();
    clearEvents();
    m_shake = 0.0f;
}
