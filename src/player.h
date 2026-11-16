#pragma once
// Igrac: pozicija, kretanje sa sudarima i animacija iz player.png.
//
// player.png je atlas 6 x 4 frejmova, svaki 16x16:
//   red 0 = hoda na dole, red 1 = levo, red 2 = desno, red 3 = gore
//   kolone 0..5 = frejmovi animacije hodanja

#include <string>
#include "raylib.h"
#include "tilemap.h"
#include "tileset.h"

// Vrednost = red u player.png
enum class Direction
{
    Down = 0,
    Left = 1,
    Right = 2,
    Up = 3,
};

class Player
{
public:
    static constexpr float HandDistance = 6.0f; // koliko je oruzje udaljeno od tela

    bool load(const std::string& texturePath);
    void unload();

    void setPosition(Vector2 position) { m_position = position; }
    Vector2 position() const { return m_position; }

    // Cita tastaturu, pomera igraca (pazeci na zidove) i vrti animaciju.
    // dt = GetFrameTime(), vreme proslog frejma u sekundama.
    void update(float dt, const Tilemap& map, const Tileset& tileset);

    // Okreni lika ka tacki u svetu (npr. ka misu dok drzi oruzje).
    // Zovi POSLE update, jer update okrece lika u pravcu kretanja.
    void faceTowards(Vector2 point);
    Direction facing() const { return m_direction; }

    void draw() const;
    void drawDebug() const; // crta hitbox, za F1 debug prikaz

    // Sredina sprite-a, kamera prati ovu tacku
    Vector2 center() const;
    // Rame: tacka oko koje se oruzje okrece (od nje se racuna ugao nisana)
    Vector2 shoulder() const;
    // Gde je drska oruzja kad lik nisani pod uglom aimAngleDeg
    Vector2 hand(float aimAngleDeg) const;
    // Trenutni hitbox (stopala), za podizanje stvari sa zemlje
    Rectangle bounds() const { return hitbox(m_position); }
    // Telo (veci pravougaonik), za udarce neprijatelja
    Rectangle body() const;

    // --- zivot ---
    float health() const { return m_health; }
    float maxHealth() const { return m_maxHealth; }
    bool isDead() const { return m_health <= 0.0f; }
    // Primi udarac. `from` = odakle je dosao (igrac odleti na suprotnu stranu).
    // Posle udarca je kratko neranjiv (trepce), da ga 5 kostura ne ubije u jednom frejmu.
    void takeDamage(float amount, Vector2 from);
    void heal(float amount);
    void resetHealth();

    // --- armor (0-10 poena: do 5 od pancira + do 5 od kacige) ---
    // Svaki poen smanjuje stetu za ArmorPerPoint (10 poena = 60% manje stete).
    static constexpr int MaxArmor = 10;
    static constexpr float ArmorPerPoint = 0.06f;
    int armor() const { return m_bodyArmor + m_helmet; }
    int bodyArmor() const { return m_bodyArmor; }
    int helmet() const { return m_helmet; }
    // Vraca false ako vec imas isto ili jace (tada se stvar ne trosi)
    bool equipBodyArmor(int points);
    bool equipHelmet(int points);
    void resetArmor() { m_bodyArmor = 0; m_helmet = 0; }

private:
    // Pravougaonik za sudare: samo stopala, ne ceo sprite.
    // Tako glava moze malo da "ude" ispred zida, kao u vecini RPG igrica.
    Rectangle hitbox(Vector2 position) const;

    static constexpr int FrameSize = 16;   // jedan frejm je 16x16
    static constexpr int FrameCount = 6;   // frejmova u jednom redu
    static constexpr float AnimationFps = 10.0f;
    static constexpr float InvulnerableTime = 0.6f; // sekundi neranjivosti posle udarca

    Texture2D m_texture{};
    Vector2 m_position{};          // gornji levi ugao sprite-a, u pikselima sveta
    Direction m_direction = Direction::Down;
    float m_speed = 70.0f;         // piksela u sekundi (1 tile = 16 piksela)

    bool m_moving = false;
    int m_frame = 0;               // trenutni frejm animacije (kolona u player.png)
    float m_animationTimer = 0.0f; // koliko je vremena proslo od zadnje promene frejma

    float m_health = 100.0f;
    float m_maxHealth = 100.0f;
    float m_hurtTimer = 0.0f;      // > 0 dok je neranjiv posle udarca
    Vector2 m_knockback{};         // brzina odbacivanja posle udarca
    int m_bodyArmor = 0;
    int m_helmet = 0;
};
