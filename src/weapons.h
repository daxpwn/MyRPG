#pragma once
// WeaponSet = weapons.png + opis svakog oruzja (iz weapons.cfg).

#include <string>
#include <vector>
#include "raylib.h"

// Vrsta oruzja = kolona "tip" u weapons.cfg
enum class WeaponType
{
    Gun,       // puska: puca metke
    Melee,     // noz: zamah izbliza
    Throwable, // granata: baca se i eksplodira
    Potion,    // napitak: vraca zivot
    BodyArmor, // pancir: daje armor
    Helmet,    // kaciga: daje armor
};

// Jedno oruzje, jedan red iz weapons.cfg
struct WeaponInfo
{
    std::string name;
    WeaponType type = WeaponType::Gun;
    int maxStack = 1;          // koliko komada staje u jedno mesto inventara
    int column = 0, row = 0;   // polje u atlasu
    Vector2 grip{};            // gde ga lik drzi (piksel u sprite-u)
    Vector2 muzzle{};          // kraj cevi (piksel u sprite-u)
    float damage = 0;
    float fireRate = 1;        // metaka u sekundi
    float bulletSpeed = 300;   // piksela u sekundi
    float spread = 0;          // stepeni
    int pellets = 1;           // metaka po pucnju
    bool automatic = false;
    float recoil = 0;          // piksela
    float range = 200;         // piksela
    // velike puske (minigun): sprite zauzima vise polja, i moze da ima frejmove
    int cellsWide = 1, cellsHigh = 1; // koliko polja atlasa zauzima sprite
    int frames = 1;            // frejmovi okretanja cevi (idu NADOLE u atlasu)
    float spinUp = 0;          // sekundi da se cevi zavrte pre prvog metka (0 = odmah puca)
    float scale = 1;           // koliko se smanji kad se crta (0.4 = 40%; pikseli ostaju, samo su manji)
    bool defined = false;
};

class WeaponSet
{
public:
    bool load(const std::string& configPath);
    void unload();

    const WeaponInfo* find(int id) const;
    // id oruzja po imenu (npr. "Glock 17"), ili -1 ako ne postoji
    int idOf(const std::string& name) const;
    // slucajna stvar iz cfg-a za drop kad ubijes protivnika (oruzje ili napitak, ne armor)
    int randomId() const;
    int count() const { return static_cast<int>(m_weapons.size()); }
    int spriteSize() const { return m_size; }

    // Ikonica oruzja upisana u pravougaonik (za hotbar i oruzje na zemlji)
    void drawIcon(int id, Rectangle dest) const;

    // Oruzje u ruci: drska je na poziciji `hand`, okrenuto za `angleDeg`.
    // flip = true kad nisani levo (inace bi oruzje bilo naopako).
    // frame = frejm animacije (za minigun: koliko su se cevi okrenule)
    void drawHeld(const WeaponInfo& weapon, Vector2 hand, float angleDeg, bool flip, int frame = 0) const;

    // Plamen iz cevi, frejm 0..flashFrames()-1
    void drawFlash(int frame, Vector2 muzzle, float angleDeg) const;
    int flashFrames() const { return m_flashFrames; }

private:
    Rectangle source(int column, int row) const;
    // deo atlasa za oruzje (pazi na velicinu u poljima i frejm)
    Rectangle source(const WeaponInfo& weapon, int frame) const;

    Texture2D m_atlas{};
    int m_size = 16;
    int m_flashColumn = 0, m_flashRow = 1, m_flashFrames = 0;
    std::vector<WeaponInfo> m_weapons; // indeks = id
};
