#include "chunks.h"

#include <cmath>

long long ChunkManager::key(int chunkX, int chunkY)
{
    // gornja 32 bita = x, donja 32 bita = y
    return (static_cast<long long>(chunkX) << 32) | static_cast<unsigned int>(chunkY);
}

void ChunkManager::update(const Camera2D& camera, const Tilemap& map, const Tileset& tileset)
{
    m_loadsThisFrame = 0;
    m_chunkPixels = ChunkTiles * tileset.tileSize();
    float chunkSize = static_cast<float>(m_chunkPixels);

    // 1) koji deo sveta kamera vidi (ugao ekrana -> koordinate sveta)
    Vector2 topLeft = GetScreenToWorld2D(Vector2{ 0.0f, 0.0f }, camera);
    Vector2 bottomRight = GetScreenToWorld2D(
        Vector2{ static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight()) }, camera);

    // 2) koji su to chunkovi (+1 chunk rezerve na svaku stranu,
    //    da se chunk ucita malo PRE nego sto se pojavi na ekranu)
    int firstX = static_cast<int>(std::floor(topLeft.x / chunkSize)) - 1;
    int firstY = static_cast<int>(std::floor(topLeft.y / chunkSize)) - 1;
    int lastX = static_cast<int>(std::floor(bottomRight.x / chunkSize)) + 1;
    int lastY = static_cast<int>(std::floor(bottomRight.y / chunkSize)) + 1;

    // ne izlazi van mape
    int mapChunksX = (map.width() + ChunkTiles - 1) / ChunkTiles;  // zaokruzeno na gore
    int mapChunksY = (map.height() + ChunkTiles - 1) / ChunkTiles;
    if (firstX < 0) firstX = 0;
    if (firstY < 0) firstY = 0;
    if (lastX > mapChunksX - 1) lastX = mapChunksX - 1;
    if (lastY > mapChunksY - 1) lastY = mapChunksY - 1;

    // 3) ucitaj one koji fale
    for (int cy = firstY; cy <= lastY; cy++)
    {
        for (int cx = firstX; cx <= lastX; cx++)
        {
            if (m_chunks.find(key(cx, cy)) == m_chunks.end())
                loadChunk(cx, cy, map, tileset);
        }
    }

    // 4) izbaci one koji su daleko. Granica je jos 1 chunk dalje od
    //    granice ucitavanja, da chunk ne bi skakao ucitan/izbacen/ucitan
    //    kad stojis tacno na ivici.
    for (auto it = m_chunks.begin(); it != m_chunks.end();)
    {
        const Chunk& chunk = it->second;
        bool far = chunk.chunkX < firstX - 1 || chunk.chunkX > lastX + 1 ||
                   chunk.chunkY < firstY - 1 || chunk.chunkY > lastY + 1;
        if (far)
        {
            unloadChunk(it->second);
            it = m_chunks.erase(it); // erase vraca sledeci element
        }
        else
        {
            ++it;
        }
    }
}

void ChunkManager::loadChunk(int chunkX, int chunkY, const Tilemap& map, const Tileset& tileset)
{
    Chunk chunk;
    chunk.chunkX = chunkX;
    chunk.chunkY = chunkY;

    // 1) kopiraj id-eve iz izvora. Polja van mape postaju -1 (prazno).
    chunk.tiles.resize(ChunkTiles * ChunkTiles);
    for (int y = 0; y < ChunkTiles; y++)
    {
        for (int x = 0; x < ChunkTiles; x++)
        {
            int column = chunkX * ChunkTiles + x;
            int row = chunkY * ChunkTiles + y;
            chunk.tiles[y * ChunkTiles + x] = map.get(column, row);
        }
    }

    // 2) "ispeci" chunk u teksturu: nacrtaj sve tile-ove jednom
    chunk.texture = LoadRenderTexture(m_chunkPixels, m_chunkPixels);
    float size = static_cast<float>(tileset.tileSize());

    BeginTextureMode(chunk.texture); // od sad crtamo u teksturu, ne na ekran
    ClearBackground(BLANK);          // providna pozadina
    for (int y = 0; y < ChunkTiles; y++)
    {
        for (int x = 0; x < ChunkTiles; x++)
        {
            tileset.drawTile(chunk.tiles[y * ChunkTiles + x], x * size, y * size);
        }
    }
    EndTextureMode();

    m_chunks[key(chunkX, chunkY)] = chunk;
    m_loadsThisFrame++;
}

void ChunkManager::unloadChunk(Chunk& chunk)
{
    if (chunk.texture.id != 0)
        UnloadRenderTexture(chunk.texture);
    chunk.texture = RenderTexture2D{};
}

void ChunkManager::clear()
{
    for (auto& entry : m_chunks)
        unloadChunk(entry.second);
    m_chunks.clear();
}

void ChunkManager::draw() const
{
    float size = static_cast<float>(m_chunkPixels);
    for (const auto& entry : m_chunks)
    {
        const Chunk& chunk = entry.second;
        // RenderTexture je u OpenGL-u okrenuta naopako, zato je visina negativna
        Rectangle source{ 0.0f, 0.0f, size, -size };
        Vector2 position{ chunk.chunkX * size, chunk.chunkY * size };
        DrawTextureRec(chunk.texture.texture, source, position, WHITE);
    }
}

void ChunkManager::drawDebug() const
{
    float size = static_cast<float>(m_chunkPixels);
    for (const auto& entry : m_chunks)
    {
        const Chunk& chunk = entry.second;
        Rectangle box{ chunk.chunkX * size, chunk.chunkY * size, size, size };
        DrawRectangleLinesEx(box, 1.0f, YELLOW);
    }
}
