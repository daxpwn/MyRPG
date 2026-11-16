#include "tileset.h"

#include <filesystem>
#include <fstream>
#include <sstream>

bool Tileset::load(const std::string& configPath)
{
    std::ifstream file(configPath);
    if (!file)
    {
        TraceLog(LOG_ERROR, "Ne mogu da otvorim %s", configPath.c_str());
        return false;
    }

    std::vector<TileInfo> tiles;
    std::string atlasName;
    int tileSize = 16;

    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line))
    {
        lineNumber++;
        // preskoci prazne redove i komentare
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream in(line);
        std::string first;
        if (!(in >> first))
            continue; // red sa samo razmacima

        if (first == "atlas")
        {
            in >> atlasName;
        }
        else if (first == "velicina")
        {
            in >> tileSize;
        }
        else
        {
            // red tabele: id ime kolona red solid podloga
            TileInfo tile;
            int id = 0;
            int solid = 0;
            try { id = std::stoi(first); }
            catch (...) { id = -1; }

            if (id < 0 || !(in >> tile.name >> tile.column >> tile.row >> solid >> tile.baseId))
            {
                TraceLog(LOG_WARNING, "%s red %d: ne razumem \"%s\"", configPath.c_str(), lineNumber, line.c_str());
                continue;
            }
            tile.solid = (solid != 0);
            tile.defined = true;

            if (id >= static_cast<int>(tiles.size()))
                tiles.resize(id + 1); // napravi mesta do ovog id-a
            tiles[id] = tile;
        }
    }

    if (atlasName.empty())
    {
        TraceLog(LOG_ERROR, "%s: nedostaje red 'atlas ime_slike.png'", configPath.c_str());
        return false;
    }

    // atlas se trazi u istom folderu gde je i cfg fajl
    std::string folder = std::filesystem::path(configPath).parent_path().string();
    std::string atlasPath = folder + "/" + atlasName;
    Texture2D atlas = LoadTexture(atlasPath.c_str());
    if (atlas.id == 0)
    {
        TraceLog(LOG_ERROR, "Ne mogu da ucitam atlas %s", atlasPath.c_str());
        return false;
    }

    // sve je proslo - tek sad zamenimo stare podatke novim
    unload();
    m_atlas = atlas;
    m_tileSize = tileSize;
    m_tiles = tiles;
    TraceLog(LOG_INFO, "Tileset: ucitano %d tile-ova iz %s", static_cast<int>(m_tiles.size()), configPath.c_str());
    return true;
}

void Tileset::unload()
{
    if (m_atlas.id != 0)
        UnloadTexture(m_atlas);
    m_atlas = Texture2D{};
    m_tiles.clear();
}

const TileInfo* Tileset::find(int id) const
{
    if (id < 0 || id >= static_cast<int>(m_tiles.size()) || !m_tiles[id].defined)
        return nullptr;
    return &m_tiles[id];
}

Rectangle Tileset::sourceRect(int column, int row) const
{
    float size = static_cast<float>(m_tileSize);
    return Rectangle{ column * size, row * size, size, size };
}

void Tileset::drawTile(int id, float x, float y) const
{
    if (id == EmptyId)
        return;

    // sirovo polje atlasa (za debug mapu)
    if (id >= AtlasIdBase)
    {
        int index = id - AtlasIdBase;
        DrawTextureRec(m_atlas, sourceRect(index % atlasColumns(), index / atlasColumns()), Vector2{ x, y }, WHITE);
        return;
    }

    const TileInfo* tile = find(id);
    if (tile == nullptr)
    {
        // nepoznat id -> crveno-crni kvadrat, da odmah vidis gresku u mapi
        int s = m_tileSize;
        DrawRectangle(static_cast<int>(x), static_cast<int>(y), s, s, MAGENTA);
        DrawRectangle(static_cast<int>(x), static_cast<int>(y), s / 2, s / 2, BLACK);
        DrawRectangle(static_cast<int>(x) + s / 2, static_cast<int>(y) + s / 2, s / 2, s / 2, BLACK);
        return;
    }

    // prvo podloga (npr. trava ispod drveta), pa tile preko nje
    const TileInfo* base = find(tile->baseId);
    if (base != nullptr)
        DrawTextureRec(m_atlas, sourceRect(base->column, base->row), Vector2{ x, y }, WHITE);

    DrawTextureRec(m_atlas, sourceRect(tile->column, tile->row), Vector2{ x, y }, WHITE);
}

int Tileset::idOf(const std::string& name, int fallback) const
{
    for (int id = 0; id < idCount(); id++)
    {
        if (m_tiles[id].defined && m_tiles[id].name == name)
            return id;
    }
    return fallback;
}

bool Tileset::isSolid(int id) const
{
    const TileInfo* tile = find(id);
    return tile != nullptr && tile->solid;
}
