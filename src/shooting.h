#pragma once
// Borba:
//   HeldWeapon   - oruzje u ruci: puska (metci, trzaj, plamen), noz (zamah),
//                  granata (bacanje)
//   BulletSystem - sve sto leti i eksplodira: metci, granate, eksplozije, cestice.
//                  Pamti i "dogadjaje stete" iz ovog frejma (udarci nozem,
//                  eksplozije), da bi neprijatelji mogli da ih procitaju.

#include <vector>
#include "raylib.h"
#include "tilemap.h"
#include "tileset.h"
#include "weapons.h"

class BulletSystem;

// Sta se desilo kad je igrac kliknuo
enum class AttackResult
{
    None,
    Shot,   // puska je opalila
    Swing,  // zamah nozem
    Thrown, // granata je bacena (inventar treba da potrosi jednu)
    Used,   // napitak/pancir/kaciga: Game odlucuje sta to radi (leci, oblaci armor)
};

class HeldWeapon
{
public:
    // Odbrojava pauzu, trzaj, plamen i zamah. Zovi svaki frejm.
    // weapon = sta je u ruci (moze nullptr), triggerHeld = drzis levi klik
    // (za minigun: dok drzis, cevi se vrte sve brze).
    void update(float dt, const WeaponInfo* weapon, bool triggerHeld);

    // Pokusa napad izabranim oruzjem.
    //   shoulder + handDistance = gde je ruka (kruzi oko ramena)
    //   aimTarget = tacka u svetu gde je mis (za granatu: gde da padne)
    AttackResult tryAttack(const WeaponInfo& weapon, bool triggerPressed, bool triggerHeld,
                           Vector2 shoulder, float handDistance, float aimDeg, Vector2 aimTarget,
                           BulletSystem& bullets);

    void draw(const WeaponSet& weapons, const WeaponInfo& weapon,
              Vector2 shoulder, float handDistance, float aimDeg) const;

    // Kad promenis oruzje: ponisti trzaj, plamen, zamah
    void reset();

private:
    static Vector2 handPosition(Vector2 shoulder, float handDistance, float angleDeg);
    static Vector2 muzzlePosition(const WeaponInfo& weapon, Vector2 hand, float angleDeg);

    void drawGun(const WeaponSet& weapons, const WeaponInfo& weapon, Vector2 shoulder, float handDistance, float aimDeg) const;
    void drawKnife(const WeaponSet& weapons, const WeaponInfo& weapon, Vector2 shoulder, float handDistance, float aimDeg) const;
    void drawGrenade(const WeaponSet& weapons, const WeaponInfo& weapon, Vector2 shoulder, float handDistance, float aimDeg) const;
    void drawUsable(const WeaponSet& weapons, const WeaponInfo& weapon, Vector2 shoulder, float handDistance, float aimDeg) const;

    static constexpr float RecoilTime = 0.12f; // koliko traje vracanje posle trzaja
    static constexpr float FlashTime = 0.06f;  // koliko traje plamen (svi frejmovi)
    static constexpr float SwingTime = 0.18f;  // koliko traje zamah nozem
    static constexpr float ThrowTime = 0.35f;  // koliko se ne vidi nova granata posle bacanja
    static constexpr float UseTime = 0.4f;     // animacija ispijanja / oblacenja
    static constexpr float SpinFrameRate = 40.0f; // frejmova cevi u sekundi pri punoj brzini
    static constexpr float SpinDownTime = 1.2f;   // koliko dugo cevi usporavaju kad pustis

    float m_cooldown = 0.0f; // sekundi do sledeceg napada
    float m_recoil = 0.0f;   // 1 = upravo opalio, pada do 0
    float m_flash = 0.0f;    // > 0 dok se vidi plamen
    float m_swing = 0.0f;    // > 0 dok traje zamah
    float m_swingSide = 1.0f; // zamah ide jednom odozgo, jednom odozdo
    float m_swingCenter = 0.0f; // ugao nisana kad je zamah poceo
    float m_throw = 0.0f;    // > 0 posle bacanja granate
    float m_use = 0.0f;      // > 0 dok traje koriscenje (napitak, armor)
    float m_spin = 0.0f;     // brzina okretanja cevi: 0 = stoje, 1 = puna brzina (moze da puca)
    float m_spinPhase = 0.0f; // koliko su se cevi okrenule (u frejmovima)
};

// Jedan metak u letu
struct Bullet
{
    Vector2 position{};
    Vector2 velocity{};  // piksela u sekundi
    float distanceLeft = 0.0f;
    float damage = 0.0f;
    bool alive = true;   // false = pogodio nesto, brise se na sledecem update-u
};

// Udarac nozem: pogadja sve u "kriski" ispred igraca (traje samo jedan frejm)
struct Strike
{
    Vector2 origin{};
    float angleDeg = 0.0f;
    float halfArcDeg = 45.0f;
    float reach = 20.0f;
    float damage = 0.0f;
};

// Eksplozija: pogadja sve u krugu (traje samo jedan frejm)
struct Blast
{
    Vector2 center{};
    float radius = 30.0f;
    float damage = 0.0f;
};

class BulletSystem
{
public:
    void spawnBullet(Vector2 position, float angleDeg, float speed, float range, float damage);
    void spawnShell(Vector2 position, float angleDeg); // caura izleti u stranu
    // Granata leti od `from` ka `target` (najdalje maxDistance) i eksplodira posle FuseTime
    void spawnGrenade(Vector2 from, Vector2 target, float maxDistance, float radius, float damage);
    void addStrike(const Strike& strike) { m_strikes.push_back(strike); }

    // Cestice kad nesto pogodis (kosti, krv...): `count` cestica u pravcu `direction`
    void spawnHitEffect(Vector2 position, Vector2 direction, Color color, int count);

    void update(float dt, const Tilemap& map, const Tileset& tileset);
    void draw() const;
    void clear();

    // Dogadjaji stete iz ovog frejma. Neprijatelji ih citaju, pa se brisu sa clearEvents().
    std::vector<Bullet>& bullets() { return m_bullets; }
    const std::vector<Strike>& strikes() const { return m_strikes; }
    const std::vector<Blast>& blasts() const { return m_blasts; }
    void clearEvents();

    // Koliko jako treba da se trese ekran zbog eksplozija (posle citanja se resetuje)
    float takeShake();

    int bulletCount() const { return static_cast<int>(m_bullets.size()); }

    static constexpr float FuseTime = 1.4f; // sekundi od bacanja do eksplozije

private:
    struct Particle
    {
        Vector2 position{};
        Vector2 velocity{};
        float life = 0.0f;    // koliko jos zivi (sekundi)
        float maxLife = 1.0f;
        float friction = 0.0f; // koliko brzo usporava (0 = ne usporava)
        float size = 1.0f;
        float grow = 0.0f;     // koliko raste u sekundi (dim)
        Color color = WHITE;
    };
    struct Grenade
    {
        Vector2 position{};      // tacka na ZEMLJI
        Vector2 velocity{};
        float height = 0.0f;     // koliko je iznad zemlje (za let u luku)
        float verticalSpeed = 0.0f;
        float fuse = FuseTime;
        float radius = 30.0f;
        float damage = 0.0f;
        float spin = 0.0f;       // okretanje u letu
    };
    struct Explosion
    {
        Vector2 center{};
        float radius = 30.0f;
        float age = 0.0f;
    };

    void spawnImpact(Vector2 position, Vector2 bulletVelocity);
    void explode(const Grenade& grenade);
    static bool blocked(Vector2 point, const Tilemap& map, const Tileset& tileset);

    std::vector<Bullet> m_bullets;
    std::vector<Particle> m_particles;
    std::vector<Grenade> m_grenades;
    std::vector<Explosion> m_explosions;
    std::vector<Strike> m_strikes;
    std::vector<Blast> m_blasts;
    float m_shake = 0.0f;
};
