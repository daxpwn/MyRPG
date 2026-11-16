#include "tilemap.h"

#include <cmath>
#include <fstream>
#include <sstream>

bool Tilemap::load(const std::string& path)
{
    std::ifstream file(path);
    if (!file)
    {
        TraceLog(LOG_ERROR, "Ne mogu da otvorim %s", path.c_str());
        return false;
    }

    std::vector<std::vector<int>> tiles;
    std::vector<Vector2> spawns;
    bool hasShopDoor = false;
    Vector2 shopDoor{};
    int startColumn = 1;
    int startRow = 1;

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream in(line);
        std::string first;
        if (!(in >> first))
            continue;

        if (first == "start")
        {
            in >> startColumn >> startRow;
            continue;
        }
        if (first == "prodavnica")
        {
            in >> shopDoor.x >> shopDoor.y;
            hasShopDoor = true;
            continue;
        }
        if (first == "spawn")
        {
            Vector2 spawn{};
            in >> spawn.x >> spawn.y;
            spawns.push_back(spawn);
            continue;
        }

        // red mape: procitaj sve brojeve u tom redu
        std::vector<int> row;
        std::istringstream all(line);
        int id = 0;
        while (all >> id)
            row.push_back(id);
        if (!all.eof())
        {
            TraceLog(LOG_ERROR, "%s: u redu \"%s\" ima nesto sto nije broj", path.c_str(), line.c_str());
            return false;
        }
        tiles.push_back(row);
    }

    if (tiles.empty())
    {
        TraceLog(LOG_ERROR, "%s: mapa je prazna", path.c_str());
        return false;
    }
    // svi redovi moraju biti iste duzine
    for (size_t r = 0; r < tiles.size(); r++)
    {
        if (tiles[r].size() != tiles[0].size())
        {
            TraceLog(LOG_ERROR, "%s: red %d mape ima %d brojeva, a prvi red %d",
                     path.c_str(), static_cast<int>(r), static_cast<int>(tiles[r].size()),
                     static_cast<int>(tiles[0].size()));
            return false;
        }
    }

    m_tiles = tiles;
    m_spawns = spawns;
    m_hasShopDoor = hasShopDoor;
    m_shopDoor = shopDoor;
    m_labels.clear();
    m_startColumn = startColumn;
    m_startRow = startRow;
    TraceLog(LOG_INFO, "Mapa: %d x %d tile-ova", width(), height());
    return true;
}

void Tilemap::create(int width, int height, int fillId)
{
    // height redova, svaki red ima width kolona popunjenih sa fillId
    m_tiles.assign(height, std::vector<int>(width, fillId));
    m_labels.clear();
    m_spawns.clear();
    m_hasShopDoor = false;
    m_startColumn = 1;
    m_startRow = 1;
}

void Tilemap::set(int column, int row, int id)
{
    if (row < 0 || row >= height() || column < 0 || column >= width())
        return;
    m_tiles[row][column] = id;
}

void Tilemap::setStart(int column, int row)
{
    m_startColumn = column;
    m_startRow = row;
}

void Tilemap::addLabel(int column, int row, const std::string& text)
{
    m_labels.push_back(MapLabel{ column, row, text });
}

int Tilemap::width() const
{
    return m_tiles.empty() ? 0 : static_cast<int>(m_tiles[0].size());
}

int Tilemap::height() const
{
    return static_cast<int>(m_tiles.size());
}

int Tilemap::get(int column, int row) const
{
    if (row < 0 || row >= height() || column < 0 || column >= width())
        return -1;
    return m_tiles[row][column];
}

void Tilemap::draw(const Tileset& tileset) const
{
    float size = static_cast<float>(tileset.tileSize());
    for (int row = 0; row < height(); row++)
    {
        for (int column = 0; column < width(); column++)
        {
            tileset.drawTile(m_tiles[row][column], column * size, row * size);
        }
    }
}

bool Tilemap::collides(Rectangle box, const Tileset& tileset) const
{
    float size = static_cast<float>(tileset.tileSize());

    // koje kolone i redove pravougaonik pokriva
    int firstColumn = static_cast<int>(std::floor(box.x / size));
    int lastColumn  = static_cast<int>(std::floor((box.x + box.width - 0.01f) / size));
    int firstRow    = static_cast<int>(std::floor(box.y / size));
    int lastRow     = static_cast<int>(std::floor((box.y + box.height - 0.01f) / size));

    for (int row = firstRow; row <= lastRow; row++)
    {
        for (int column = firstColumn; column <= lastColumn; column++)
        {
            int id = get(column, row);
            if (id == Tileset::EmptyId)  // van mape ili prazno = zid
                return true;
            if (tileset.isSolid(id))
                return true;
        }
    }
    return false;
}

Vector2 Tilemap::startPosition(const Tileset& tileset) const
{
    float size = static_cast<float>(tileset.tileSize());
    return Vector2{ m_startColumn * size, m_startRow * size };
}
