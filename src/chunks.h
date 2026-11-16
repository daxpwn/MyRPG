#pragma once
// ChunkManager: mapa se deli na chunkove (kvadrate od 16x16 tile-ova).
//
// Svaki frejm gledamo sta kamera vidi:
//   - chunk koji je u vidnom polju (+1 chunk rezerve) a nije ucitan -> UCITAJ
//   - chunk koji je daleko od vidnog polja                          -> IZBACI
//
// "Ucitati" chunk znaci:
//   1) kopirati njegovih 16x16 id-eva iz izvora (sada: Tilemap u memoriji)
//   2) jednom ga nacrtati u sopstvenu teksturu (RenderTexture2D, "bake")
// Posle toga chunk se crta kao JEDNA slika, umesto 256+ malih tile-ova.
//
// Za ogroman svet bi se u loadChunk() citao fajl sa diska (npr. chunk_3_5.txt)
// umesto iz Tilemap-a - ostatak sistema ostaje isti.

#include <unordered_map>
#include <vector>
#include "raylib.h"
#include "tilemap.h"
#include "tileset.h"

class ChunkManager
{
public:
    static constexpr int ChunkTiles = 16; // chunk je 16x16 tile-ova

    // Napomena: nema destruktora koji brise teksture, jer bi se pozvao tek
    // POSLE CloseWindow (a tada vise nema grafickog konteksta).
    // Zato se clear() zove rucno pre CloseWindow.

    // Ucita chunkove koji se vide i izbaci one koji su daleko.
    // MORA se zvati PRE BeginDrawing (crta u teksture).
    void update(const Camera2D& camera, const Tilemap& map, const Tileset& tileset);

    void draw() const;      // crta ucitane chunkove (unutar BeginMode2D)
    void drawDebug() const; // zute ivice ucitanih chunkova

    // Izbaci sve chunkove (kad se promeni mapa ili tileset)
    void clear();

    int loadedCount() const { return static_cast<int>(m_chunks.size()); }
    int loadsThisFrame() const { return m_loadsThisFrame; }

private:
    struct Chunk
    {
        int chunkX = 0;            // koordinate chunka (ne tile-a!)
        int chunkY = 0;
        std::vector<int> tiles;    // ChunkTiles * ChunkTiles id-eva, red po red
        RenderTexture2D texture{}; // "pecena" slika celog chunka
    };

    // Kljuc za mapu chunkova: dve int koordinate spakovane u jedan broj
    static long long key(int chunkX, int chunkY);

    void loadChunk(int chunkX, int chunkY, const Tilemap& map, const Tileset& tileset);
    static void unloadChunk(Chunk& chunk);

    std::unordered_map<long long, Chunk> m_chunks;
    int m_chunkPixels = 0;    // velicina chunka u pikselima sveta
    int m_loadsThisFrame = 0;
};
