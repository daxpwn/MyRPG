#include "weapons.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

bool WeaponSet::load(const std::string& configPath)
{
    std::ifstream file(configPath);
    if (!file)
    {
        TraceLog(LOG_ERROR, "Ne mogu da otvorim %s", configPath.c_str());
        return false;
    }

    std::vector<WeaponInfo> weapons;
    std::string atlasName;
    int size = 16;
    int flashColumn = 0, flashRow = 1, flashFrames = 0;

    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line))
    {
        lineNumber++;
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream in(line);
        std::string first;
        if (!(in >> first))
            continue;

        if (first == "atlas")
            in >> atlasName;
        else if (first == "velicina")
            in >> size;
        else if (first == "plamen")
            in >> flashColumn >> flashRow >> flashFrames;
        else
        {
            WeaponInfo w;
            int id = -1;
            int automatic = 0;
            try { id = std::stoi(first); }
            catch (...) { id = -1; }

            bool ok = id >= 0 &&
                      (in >> w.name >> w.column >> w.row >> w.grip.x >> w.grip.y >> w.muzzle.x >> w.muzzle.y
                          >> w.damage >> w.fireRate >> w.bulletSpeed >> w.spread >> w.pellets >> automatic
                          >> w.recoil >> w.range);
            // tip je poslednja kolona; ako fali, oruzje je puska
            std::string type = "puska";
            if (ok)
                in >> type;
            // Opcione kolone posle tipa (samo za velike puske):
            //   sirina visina (u poljima)  frejmova  zagrevanje (sekundi)  skala
            // Citamo u privremene promenljive, pa ako kolona fali ostaje podrazumevano.
            int wide = 1, high = 1, frames = 1;
            float spinUp = 0.0f, scale = 1.0f;
            if (ok && in >> wide >> high)
            {
                w.cellsWide = std::max(1, wide);
                w.cellsHigh = std::max(1, high);
                if (in >> frames)
                    w.frames = std::max(1, frames);
                if (in >> spinUp)
                    w.spinUp = std::max(0.0f, spinUp);
                if (in >> scale && scale > 0.0f)
                    w.scale = scale;
            }
            if (!ok)
            {
                TraceLog(LOG_WARNING, "%s red %d: ne razumem \"%s\"", configPath.c_str(), lineNumber, line.c_str());
                continue;
            }
            // U cfg fajlu ime ne sme imati razmak (razmak odvaja kolone),
            // pa se pise Desert_Eagle, a u igri prikazujemo "Desert Eagle".
            std::replace(w.name.begin(), w.name.end(), '_', ' ');
            w.automatic = (automatic != 0);
            if (type == "noz")
                w.type = WeaponType::Melee;
            else if (type == "granata")
                w.type = WeaponType::Throwable;
            else if (type == "lek")
                w.type = WeaponType::Potion;
            else if (type == "pancir")
                w.type = WeaponType::BodyArmor;
            else if (type == "kaciga")
                w.type = WeaponType::Helmet;
            bool stacks = (w.type == WeaponType::Throwable || w.type == WeaponType::Potion);
            w.maxStack = stacks ? 5 : 1;
            if (w.fireRate <= 0) w.fireRate = 1;
            if (w.pellets < 1) w.pellets = 1;
            w.defined = true;

            if (id >= static_cast<int>(weapons.size()))
                weapons.resize(id + 1);
            weapons[id] = w;
        }
    }

    std::string folder = std::filesystem::path(configPath).parent_path().string();
    std::string atlasPath = folder + "/" + atlasName;
    Texture2D atlas = LoadTexture(atlasPath.c_str());
    if (atlas.id == 0)
    {
        TraceLog(LOG_ERROR, "Ne mogu da ucitam %s", atlasPath.c_str());
        return false;
    }

    unload();
    m_atlas = atlas;
    m_size = size;
    m_flashColumn = flashColumn;
    m_flashRow = flashRow;
    m_flashFrames = flashFrames;
    m_weapons = weapons;
    TraceLog(LOG_INFO, "Oruzja: ucitano %d iz %s", count(), configPath.c_str());
    return true;
}

void WeaponSet::unload()
{
    if (m_atlas.id != 0)
        UnloadTexture(m_atlas);
    m_atlas = Texture2D{};
    m_weapons.clear();
}

const WeaponInfo* WeaponSet::find(int id) const
{
    if (id < 0 || id >= count() || !m_weapons[id].defined)
        return nullptr;
    return &m_weapons[id];
}

int WeaponSet::idOf(const std::string& name) const
{
    for (int id = 0; id < count(); id++)
    {
        if (m_weapons[id].defined && m_weapons[id].name == name)
            return id;
    }
    return -1;
}

int WeaponSet::randomId() const
{
    // skupi sve id-eve koji postoje (bez armora), pa izaberi jedan
    std::vector<int> ids;
    for (int id = 0; id < count(); id++)
    {
        const WeaponInfo& w = m_weapons[id];
        if (w.defined && w.type != WeaponType::BodyArmor && w.type != WeaponType::Helmet)
            ids.push_back(id);
    }
    if (ids.empty())
        return -1;
    return ids[GetRandomValue(0, static_cast<int>(ids.size()) - 1)];
}

Rectangle WeaponSet::source(int column, int row) const
{
    float s = static_cast<float>(m_size);
    return Rectangle{ column * s, row * s, s, s };
}

Rectangle WeaponSet::source(const WeaponInfo& weapon, int frame) const
{
    float s = static_cast<float>(m_size);
    frame = weapon.frames > 0 ? frame % weapon.frames : 0;
    // frejmovi stoje jedan ispod drugog: svaki je cellsHigh polja nize
    int row = weapon.row + frame * weapon.cellsHigh;
    return Rectangle{ weapon.column * s, row * s, weapon.cellsWide * s, weapon.cellsHigh * s };
}

void WeaponSet::drawIcon(int id, Rectangle dest) const
{
    const WeaponInfo* w = find(id);
    if (w == nullptr)
        return;
    Rectangle src = source(*w, 0);
    // Siroka slika (minigun 32x16) se smanji da stane u kvadrat, bez razvlacenja:
    // uzmemo manji od dva odnosa i centriramo.
    float scale = std::min(dest.width / src.width, dest.height / src.height);
    float width = src.width * scale;
    float height = src.height * scale;
    Rectangle fitted{ dest.x + (dest.width - width) / 2.0f, dest.y + (dest.height - height) / 2.0f, width, height };
    DrawTexturePro(m_atlas, src, fitted, Vector2{ 0, 0 }, 0.0f, WHITE);
}

void WeaponSet::drawHeld(const WeaponInfo& weapon, Vector2 hand, float angleDeg, bool flip, int frame) const
{
    Rectangle src = source(weapon, frame);
    // skala: velika slika (minigun 96x32) se crta manja, npr. 0.4 -> 38x13 piksela sveta
    float width = src.width * weapon.scale, height = src.height * weapon.scale;
    // tacka oko koje se okrece = drska (i ona se smanji zajedno sa slikom)
    Vector2 origin{ weapon.grip.x * weapon.scale, weapon.grip.y * weapon.scale };
    if (flip)
    {
        // Negativna visina = slika okrenuta naopako (po vertikali).
        // Kad nisanis levo i okrenes za ~180 stepeni, ovo je vraca "na noge".
        src.height = -src.height;
        origin.y = height - origin.y;
    }
    DrawTexturePro(m_atlas, src, Rectangle{ hand.x, hand.y, width, height }, origin, angleDeg, WHITE);
}

void WeaponSet::drawFlash(int frame, Vector2 muzzle, float angleDeg) const
{
    if (frame < 0 || frame >= m_flashFrames)
        return;
    float s = static_cast<float>(m_size);
    Rectangle src = source(m_flashColumn + frame, m_flashRow);
    // plamen je nacrtan tako da mu je "pocetak" na levoj ivici, na sredini visine
    DrawTexturePro(m_atlas, src, Rectangle{ muzzle.x, muzzle.y, s, s }, Vector2{ 0.0f, s / 2.0f }, angleDeg, WHITE);
}
