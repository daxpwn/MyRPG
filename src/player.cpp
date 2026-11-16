#include "player.h"

#include <algorithm>
#include <cmath>

bool Player::load(const std::string& texturePath)
{
    m_texture = LoadTexture(texturePath.c_str());
    if (m_texture.id == 0)
    {
        TraceLog(LOG_ERROR, "Ne mogu da ucitam %s", texturePath.c_str());
        return false;
    }
    return true;
}

void Player::unload()
{
    if (m_texture.id != 0)
        UnloadTexture(m_texture);
    m_texture = Texture2D{};
}

void Player::update(float dt, const Tilemap& map, const Tileset& tileset)
{
    // --- 1) ulaz: u kom pravcu igrac hoce da ide ---
    Vector2 input{ 0.0f, 0.0f };
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) input.x += 1.0f;
    if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) input.x -= 1.0f;
    if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) input.y += 1.0f;
    if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) input.y -= 1.0f;

    m_moving = (input.x != 0.0f || input.y != 0.0f);

    if (m_moving)
    {
        // Normalizacija: bez ovoga bi dijagonalno kretanje bilo ~1.41x brze
        float length = std::sqrt(input.x * input.x + input.y * input.y);
        input.x /= length;
        input.y /= length;

        // Smer u koji lik gleda (bira red u player.png)
        if (input.x < 0.0f)      m_direction = Direction::Left;
        else if (input.x > 0.0f) m_direction = Direction::Right;
        else if (input.y < 0.0f) m_direction = Direction::Up;
        else                     m_direction = Direction::Down;
    }

    // Pomeraj = kretanje + odbacivanje od udarca (koje se brzo smiruje)
    Vector2 move{ input.x * m_speed + m_knockback.x, input.y * m_speed + m_knockback.y };
    float slow = std::max(0.0f, 1.0f - 10.0f * dt);
    m_knockback.x *= slow;
    m_knockback.y *= slow;
    if (m_hurtTimer > 0.0f)
        m_hurtTimer -= dt;

    // --- 2) kretanje sa sudarima ---
    // X i Y pomeramo posebno. Tako kad udaris u zid ukoso,
    // i dalje "klizis" duz njega umesto da se zalepis.
    Vector2 next = m_position;
    next.x += move.x * dt;
    if (!map.collides(hitbox(next), tileset))
        m_position.x = next.x;

    next = m_position;
    next.y += move.y * dt;
    if (!map.collides(hitbox(next), tileset))
        m_position.y = next.y;

    // --- 3) animacija ---
    if (m_moving)
    {
        m_animationTimer += dt;
        if (m_animationTimer >= 1.0f / AnimationFps)
        {
            m_animationTimer = 0.0f;
            m_frame = (m_frame + 1) % FrameCount; // 0,1,2,3,4,5,0,1,...
        }
    }
    else
    {
        m_frame = 0; // stoji -> prvi frejm
        m_animationTimer = 0.0f;
    }
}

void Player::draw() const
{
    // Koji deo player.png crtamo: kolona = frejm, red = smer
    Rectangle source{
        static_cast<float>(m_frame * FrameSize),
        static_cast<float>(static_cast<int>(m_direction) * FrameSize),
        static_cast<float>(FrameSize),
        static_cast<float>(FrameSize),
    };
    // Zaokruzivanje na ceo piksel, da pixel art ne "treperi"
    Vector2 position{ std::round(m_position.x), std::round(m_position.y) };

    // Posle udarca: trepce (svaki drugi "tik" je crven)
    Color tint = WHITE;
    if (m_hurtTimer > 0.0f && std::fmod(m_hurtTimer, 0.16f) < 0.08f)
        tint = Color{ 255, 90, 90, 255 };
    DrawTextureRec(m_texture, source, position, tint);
}

Rectangle Player::body() const
{
    return Rectangle{ m_position.x + 4.0f, m_position.y + 3.0f, 8.0f, 13.0f };
}

void Player::takeDamage(float amount, Vector2 from)
{
    if (m_hurtTimer > 0.0f || isDead())
        return; // jos je neranjiv od proslog udarca
    // armor smanjuje stetu: 6% po poenu
    amount *= 1.0f - ArmorPerPoint * armor();
    m_health = std::max(0.0f, m_health - amount);
    m_hurtTimer = InvulnerableTime;

    // odleti od napadaca
    Vector2 c = center();
    float dx = c.x - from.x, dy = c.y - from.y;
    float length = std::sqrt(dx * dx + dy * dy);
    if (length > 0.01f)
    {
        m_knockback.x = dx / length * 160.0f;
        m_knockback.y = dy / length * 160.0f;
    }
}

void Player::heal(float amount)
{
    m_health = std::min(m_maxHealth, m_health + amount);
}

bool Player::equipBodyArmor(int points)
{
    if (m_bodyArmor >= points)
        return false;
    m_bodyArmor = std::min(points, MaxArmor / 2);
    return true;
}

bool Player::equipHelmet(int points)
{
    if (m_helmet >= points)
        return false;
    m_helmet = std::min(points, MaxArmor / 2);
    return true;
}

void Player::resetHealth()
{
    m_health = m_maxHealth;
    m_hurtTimer = 0.0f;
    m_knockback = Vector2{};
}

void Player::drawDebug() const
{
    Rectangle box = hitbox(m_position);
    DrawRectangleLinesEx(box, 0.5f, RED);
}

Vector2 Player::center() const
{
    return Vector2{ m_position.x + FrameSize / 2.0f, m_position.y + FrameSize / 2.0f };
}

Vector2 Player::shoulder() const
{
    // sredina tela, malo ispod centra sprite-a (visina ramena/ruku)
    return Vector2{ m_position.x + FrameSize / 2.0f, m_position.y + 10.0f };
}

Vector2 Player::hand(float aimAngleDeg) const
{
    // Ruka "kruzi" oko ramena: uvek je HandDistance piksela od tela, u pravcu nisana.
    // Tako oruzje stoji SA STRANE lika umesto preko njega.
    Vector2 s = shoulder();
    float a = aimAngleDeg * DEG2RAD;
    return Vector2{ std::round(s.x + std::cos(a) * HandDistance), std::round(s.y + std::sin(a) * HandDistance) };
}

void Player::faceTowards(Vector2 point)
{
    Vector2 c = center();
    float dx = point.x - c.x;
    float dy = point.y - c.y;
    // koja osa je "jaca" odredjuje smer
    if (std::fabs(dx) > std::fabs(dy))
        m_direction = (dx < 0.0f) ? Direction::Left : Direction::Right;
    else
        m_direction = (dy < 0.0f) ? Direction::Up : Direction::Down;
}

Rectangle Player::hitbox(Vector2 position) const
{
    // stopala: 8 piksela siroko, 5 visoko, pri dnu sprite-a
    return Rectangle{ position.x + 4.0f, position.y + 11.0f, 8.0f, 5.0f };
}
