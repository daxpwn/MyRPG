#include "raylib.h"
#include "game.h"

// main samo pravi prozor i vrti petlju. Sve ostalo je u Game (game.h / game.cpp).
//
// Game loop: jedan prolaz petlje = jedan frejm (~60 puta u sekundi):
//   1) ulaz   2) update   3) crtanje       -> sve to radi game.runFrame()
int main()
{
    // Flagovi se postavljaju PRE InitWindow
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(800, 600, "MY RPG");
    SetWindowMinSize(400, 300);
    SetTargetFPS(60);
    SetExitKey(KEY_NULL); // Esc vise ne gasi igru, nego otvara pauzu

    // Teksture se mogu ucitati tek POSLE InitWindow, zato load() ide ovde
    Game game;
    if (!game.load())
    {
        // Greska je ispisana u konzoli (crni prozor iza igre).
        CloseWindow();
        return 1;
    }

    while (!WindowShouldClose() && !game.wantsToQuit())
        game.runFrame();

    game.unload(); // sve sa graficke PRE CloseWindow
    CloseWindow();
    return 0;
}

// ============================================================================
//  PODSETNIK: najkorisnije raylib funkcije za 2D igricu
//  (pun spisak: build/_deps/raylib-src/src/raylib.h
//   ili https://www.raylib.com/cheatsheet/cheatsheet.html)
// ============================================================================
//
// --- TIPOVI KOJE CES CESTO KORISTITI ----------------------------------------
//   Color      { r, g, b, a }               boja, 0-255   npr. Color{255, 0, 0, 255}
//   Vector2    { x, y }                     tacka ili pravac (float)
//   Rectangle  { x, y, width, height }      pravougaonik (float)
//   Texture2D                               slika ucitana na grafickoj
//
//   Gotove boje: WHITE, BLACK, RAYWHITE, GRAY, DARKGRAY, LIGHTGRAY, RED,
//   MAROON, GREEN, LIME, DARKGREEN, BLUE, SKYBLUE, DARKBLUE, YELLOW, GOLD,
//   ORANGE, PINK, PURPLE, VIOLET, BROWN, BEIGE, MAGENTA, BLANK (providno)
//
// --- PROZOR -----------------------------------------------------------------
//   void InitWindow(int width, int height, const char *title)   otvori prozor (prvo ovo!)
//   void CloseWindow()                                           zatvori prozor (na kraju)
//   bool WindowShouldClose()                                     X kliknut ili ESC?
//   void SetWindowTitle(const char *title)                       promeni naslov
//   void SetWindowSize(int width, int height)                    promeni velicinu
//   void SetWindowPosition(int x, int y)                         pomeri prozor na monitoru
//   int  GetScreenWidth() / GetScreenHeight()                    trenutna velicina prozora
//   void ToggleFullscreen()                                      fullscreen <-> prozor
//   bool IsWindowFullscreen()
//   bool IsWindowFocused()                                       da li je prozor aktivan
//   bool IsWindowResized()                                       da li je promenjena velicina
//   void SetConfigFlags(unsigned int flags)                      PRE InitWindow, npr.
//                                                                SetConfigFlags(FLAG_WINDOW_RESIZABLE);
//   void ShowCursor() / HideCursor()                             prikazi/sakrij strelicu misa
//   void SetExitKey(int key)                                     koji taster gasi igru (KEY_NULL = nijedan)
//   void TakeScreenshot(const char *fileName)                    npr. "slika.png"
//
// --- VREME I FPS ------------------------------------------------------------
//   void   SetTargetFPS(int fps)          max broj frejmova u sekundi
//   float  GetFrameTime()                 koliko je sekundi trajao prosli frame (delta time)
//   double GetTime()                      sekundi od InitWindow
//   int    GetFPS()                       trenutni FPS
//   void   DrawFPS(int x, int y)          ispise FPS na ekranu
//   int    GetRandomValue(int min, int max)   slucajan broj (min i max ukljuceni)
//
// --- CRTANJE: OKVIR ---------------------------------------------------------
//   void BeginDrawing()                   pocni frame
//   void EndDrawing()                     zavrsi frame i prikazi ga
//   void ClearBackground(Color color)     obrisi ekran jednom bojom
//   void BeginMode2D(Camera2D camera)     sve izmedju ovoga i EndMode2D() se crta
//   void EndMode2D()                      kroz kameru (pomeranje, zum)
//
// --- CRTANJE: OBLICI --------------------------------------------------------
//   void DrawPixel(int x, int y, Color color)
//   void DrawLine(int x1, int y1, int x2, int y2, Color color)
//   void DrawLineEx(Vector2 start, Vector2 end, float thick, Color color)       deblja linija
//   void DrawCircle(int centerX, int centerY, float radius, Color color)
//   void DrawCircleV(Vector2 center, float radius, Color color)
//   void DrawCircleLines(int centerX, int centerY, float radius, Color color)   samo obod
//   void DrawRectangle(int x, int y, int width, int height, Color color)
//   void DrawRectangleV(Vector2 position, Vector2 size, Color color)
//   void DrawRectangleRec(Rectangle rec, Color color)
//   void DrawRectangleLines(int x, int y, int width, int height, Color color)   samo ivica
//   void DrawRectangleRounded(Rectangle rec, float roundness, int segments, Color color)
//   void DrawTriangle(Vector2 v1, Vector2 v2, Vector2 v3, Color color)          temena suprotno od kazaljke!
//   void DrawPoly(Vector2 center, int sides, float radius, float rotation, Color color)
//
// --- TEKST ------------------------------------------------------------------
//   void DrawText(const char *text, int x, int y, int fontSize, Color color)
//   int  MeasureText(const char *text, int fontSize)      sirina teksta u pikselima
//                                                         (korisno za centriranje)
//   const char *TextFormat(const char *text, ...)         kao printf, npr.
//                                                         DrawText(TextFormat("Poeni: %d", poeni), 10, 10, 20, WHITE);
//   Font LoadFont(const char *fileName)                   ucitaj svoj font (.ttf)
//   void DrawTextEx(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint)
//
// --- SLIKE (TEKSTURE) -------------------------------------------------------
//   Texture2D LoadTexture(const char *fileName)           ucitaj .png (POSLE InitWindow)
//   void UnloadTexture(Texture2D texture)                 oslobodi (PRE CloseWindow)
//   void DrawTexture(Texture2D texture, int x, int y, Color tint)        tint = WHITE za originalne boje
//   void DrawTextureV(Texture2D texture, Vector2 position, Color tint)
//   void DrawTextureEx(Texture2D texture, Vector2 position, float rotation, float scale, Color tint)
//   void DrawTextureRec(Texture2D texture, Rectangle source, Vector2 position, Color tint)
//                                                         crta deo slike (za sprite sheet / animacije)
//   void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint)
//
// --- TASTATURA --------------------------------------------------------------
//   bool IsKeyDown(int key)          drzi se pritisnut (za kretanje)
//   bool IsKeyPressed(int key)       pritisnut BAS u ovom frejmu (za skok, pucanje, meni)
//   bool IsKeyReleased(int key)      pusten u ovom frejmu
//   int  GetKeyPressed()             koji taster je pritisnut (0 ako nijedan)
//
//   Tasteri: KEY_A ... KEY_Z, KEY_ZERO ... KEY_NINE, KEY_SPACE, KEY_ENTER,
//   KEY_ESCAPE, KEY_LEFT_SHIFT, KEY_LEFT_CONTROL, KEY_TAB, KEY_BACKSPACE,
//   KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_F1 ... KEY_F12
//
// --- MIS --------------------------------------------------------------------
//   Vector2 GetMousePosition()                 gde je mis (u pikselima prozora)
//   bool    IsMouseButtonDown(int button)      drzi se
//   bool    IsMouseButtonPressed(int button)   kliknut u ovom frejmu
//   float   GetMouseWheelMove()                tockic: + gore, - dole
//
//   Dugmad: MOUSE_BUTTON_LEFT, MOUSE_BUTTON_RIGHT, MOUSE_BUTTON_MIDDLE
//
// --- SUDARI (COLLISION) -----------------------------------------------------
//   bool CheckCollisionRecs(Rectangle a, Rectangle b)                      dva pravougaonika
//   bool CheckCollisionCircles(Vector2 c1, float r1, Vector2 c2, float r2) dva kruga
//   bool CheckCollisionCircleRec(Vector2 center, float radius, Rectangle rec)
//   bool CheckCollisionPointRec(Vector2 point, Rectangle rec)              npr. mis nad dugmetom
//   Rectangle GetCollisionRec(Rectangle a, Rectangle b)                    deo gde se preklapaju
//
// --- BOJE -------------------------------------------------------------------
//   Color Fade(Color color, float alpha)       ista boja, providnost 0.0f - 1.0f
//
// --- ZVUK -------------------------------------------------------------------
//   void  InitAudioDevice()                    pozovi jednom posle InitWindow
//   void  CloseAudioDevice()                   pre CloseWindow
//   Sound LoadSound(const char *fileName)      kratki zvukovi (.wav, .ogg, .mp3)
//   void  PlaySound(Sound sound)
//   void  UnloadSound(Sound sound)
//   Music LoadMusicStream(const char *fileName)   duga muzika u pozadini
//   void  PlayMusicStream(Music music)
//   void  UpdateMusicStream(Music music)       MORA svaki frame u game loop-u!
//   void  UnloadMusicStream(Music music)
// ============================================================================
