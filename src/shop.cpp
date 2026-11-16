#include "shop.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

// Raspored sobe (u tile-ovima)
static constexpr int PerRow = 5;     // stvari u jednom redu
static constexpr int SpacingX = 4;   // razmak izmedju stvari (mesta za ime)
static constexpr int SpacingY = 4;   // stvar, ime, cena, prazan red
static constexpr int FirstColumn = 3;
static constexpr int ItemsTop = 5;   // ispod pulta

bool Shop::load(const std::string& configPath, const WeaponSet& weapons)
{
    std::ifstream file(configPath);
    if (!file)
    {
        TraceLog(LOG_ERROR, "Ne mogu da otvorim %s", configPath.c_str());
        return false;
    }

    std::vector<Offer> offers;
    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;
        std::istringstream in(line);
        std::string name;
        Offer offer;
        if (!(in >> name >> offer.price >> offer.count))
            continue;
        std::replace(name.begin(), name.end(), '_', ' '); // isto kao u weapons.cfg
        offer.itemId = weapons.idOf(name);
        if (offer.itemId < 0)
        {
            TraceLog(LOG_WARNING, "%s: nema stvari \"%s\" u weapons.cfg", configPath.c_str(), name.c_str());
            continue;
        }
        offers.push_back(offer);
    }
    m_offers = offers;
    TraceLog(LOG_INFO, "Prodavnica: %d stvari u ponudi", static_cast<int>(m_offers.size()));
    return true;
}

void Shop::build(const Tileset& tileset)
{
    int count = static_cast<int>(m_offers.size());
    int rows = std::max(1, (count + PerRow - 1) / PerRow);
    int width = FirstColumn * 2 + (PerRow - 1) * SpacingX + 1;
    int lastItemRow = ItemsTop + (rows - 1) * SpacingY;
    int height = lastItemRow + 6; // imena, cene, prolaz, vrata

    // tile-ovi po imenu iz tiles.cfg (ako neki fali, uzme se nesto drugo)
    int floor = tileset.idOf("daske", 0);
    int wall = tileset.idOf("cigla", tileset.idOf("kameni_zid", floor));
    int door = tileset.idOf("vrata", floor);
    int counter = tileset.idOf("sanduk", wall);

    m_map.create(width, height, floor);
    for (int x = 0; x < width; x++)
    {
        m_map.set(x, 0, wall);
        m_map.set(x, height - 1, wall);
    }
    for (int y = 0; y < height; y++)
    {
        m_map.set(0, y, wall);
        m_map.set(width - 1, y, wall);
    }

    // prodavac iza pulta, gore na sredini
    m_keeperColumn = width / 2;
    m_keeperRow = 1;
    for (int x = m_keeperColumn - 2; x <= m_keeperColumn + 2; x++)
        m_map.set(x, 2, counter);

    // vrata (izlaz) dole na sredini
    m_doorColumn = width / 2;
    m_doorRow = height - 1;
    m_map.set(m_doorColumn, m_doorRow, door);
    m_map.setStart(m_doorColumn, m_doorRow - 2);

    for (int i = 0; i < count; i++)
    {
        m_offers[i].column = FirstColumn + (i % PerRow) * SpacingX;
        m_offers[i].row = ItemsTop + (i / PerRow) * SpacingY;
    }
}

Vector2 Shop::entrancePosition(const Tileset& tileset) const
{
    float tile = static_cast<float>(tileset.tileSize());
    return Vector2{ m_doorColumn * tile, (m_doorRow - 2) * tile };
}

Rectangle Shop::exitDoor(const Tileset& tileset) const
{
    float tile = static_cast<float>(tileset.tileSize());
    return Rectangle{ m_doorColumn * tile, m_doorRow * tile, tile, tile };
}

int Shop::nearestOffer(Vector2 point, const Tileset& tileset) const
{
    float tile = static_cast<float>(tileset.tileSize());
    int best = -1;
    float bestDistance = 14.0f; // mora da stoji skoro na stvari
    for (int i = 0; i < static_cast<int>(m_offers.size()); i++)
    {
        float cx = m_offers[i].column * tile + tile / 2.0f;
        float cy = m_offers[i].row * tile + tile / 2.0f;
        float distance = std::sqrt((point.x - cx) * (point.x - cx) + (point.y - cy) * (point.y - cy));
        if (distance < bestDistance)
        {
            bestDistance = distance;
            best = i;
        }
    }
    return best;
}

std::string Shop::prompt(int offer, int money, const WeaponSet& weapons) const
{
    const Offer& o = m_offers[offer];
    const WeaponInfo* info = weapons.find(o.itemId);
    if (info == nullptr)
        return "";
    std::string text = "E: kupi " + info->name;
    if (o.count > 1)
        text += " x" + std::to_string(o.count);
    text += " za " + std::to_string(o.price) + " novcica";
    if (money < o.price)
        text += "  (nemas dovoljno)";
    return text;
}

std::string Shop::buy(int offer, int& money, Inventory& inventory, Player& player,
                      const WeaponSet& weapons, bool& success) const
{
    success = false;
    const Offer& o = m_offers[offer];
    const WeaponInfo* info = weapons.find(o.itemId);
    if (info == nullptr)
        return "";
    if (money < o.price)
        return "Nemas dovoljno novcica (treba " + std::to_string(o.price) + ")";

    int points = static_cast<int>(info->damage); // za armor: koliko poena daje
    if (info->type == WeaponType::BodyArmor)
    {
        if (!player.equipBodyArmor(points))
            return "Vec imas pancir";
    }
    else if (info->type == WeaponType::Helmet)
    {
        if (!player.equipHelmet(points))
            return "Vec imas kacigu";
    }
    else
    {
        if (inventory.spaceFor(o.itemId, weapons) < o.count)
            return "Nemas mesta u inventaru (Q baca stvar)";
        inventory.add(o.itemId, o.count, weapons);
    }

    money -= o.price;
    success = true;
    return "Kupljeno: " + info->name;
}

void Shop::draw(const Tileset& tileset, const WeaponSet& weapons, int highlighted, float time) const
{
    float tile = static_cast<float>(tileset.tileSize());

    // prodavac (lik iz atlasa, kolona 0 red 1)
    tileset.drawTile(tileset.atlasId(0, 1), m_keeperColumn * tile, m_keeperRow * tile);

    for (int i = 0; i < static_cast<int>(m_offers.size()); i++)
    {
        const Offer& o = m_offers[i];
        float x = o.column * tile;
        float y = o.row * tile;

        // tepih ispod stvari; izabrana stvar ima zlatni okvir koji "pulsira"
        DrawRectangleRec(Rectangle{ x - 1.0f, y - 1.0f, tile + 2.0f, tile + 2.0f }, Color{ 90, 40, 40, 255 });
        DrawRectangleLinesEx(Rectangle{ x - 1.0f, y - 1.0f, tile + 2.0f, tile + 2.0f }, 1.0f, Color{ 150, 110, 60, 255 });
        if (i == highlighted)
        {
            float pulse = 0.6f + 0.4f * std::sin(time * 8.0f);
            DrawRectangleLinesEx(Rectangle{ x - 2.0f, y - 2.0f, tile + 4.0f, tile + 4.0f }, 1.0f, Fade(GOLD, pulse));
        }

        float bob = std::round(std::sin(time * 3.0f + i) * 1.0f);
        weapons.drawIcon(o.itemId, Rectangle{ x, y - 1.0f + bob, tile, tile });
    }
}

void Shop::drawLabels(const Camera2D& camera, const Tileset& tileset, const WeaponSet& weapons,
                      int money, float uiScale) const
{
    float tile = static_cast<float>(tileset.tileSize());
    int font = 10 * std::max(1, static_cast<int>(std::lround(uiScale * 2.0f / 3.0f)));

    for (const Offer& o : m_offers)
    {
        const WeaponInfo* info = weapons.find(o.itemId);
        if (info == nullptr)
            continue;
        // tacka ispod sredine stvari, prevedena u ekran
        Vector2 screen = GetWorldToScreen2D(Vector2{ o.column * tile + tile / 2.0f, (o.row + 1) * tile + 2.0f }, camera);
        if (screen.y < -50 || screen.y > GetScreenHeight() + 50)
            continue;

        std::string name = info->name + (o.count > 1 ? " x" + std::to_string(o.count) : "");
        int nameW = MeasureText(name.c_str(), font);
        int x = static_cast<int>(screen.x) - nameW / 2;
        int y = static_cast<int>(screen.y);
        DrawText(name.c_str(), x + 1, y + 1, font, BLACK);
        DrawText(name.c_str(), x, y, font, WHITE);

        const char* price = TextFormat("%d novcica", o.price);
        int priceW = MeasureText(price, font);
        Color color = money >= o.price ? GOLD : Color{ 230, 80, 70, 255 };
        DrawText(price, static_cast<int>(screen.x) - priceW / 2 + 1, y + font + 3, font, BLACK);
        DrawText(price, static_cast<int>(screen.x) - priceW / 2, y + font + 2, font, color);
    }
}
