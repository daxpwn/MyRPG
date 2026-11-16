#include "menu.h"

#include <algorithm>
#include <cmath>

MainMenu::MainMenu()
{
    m_options = {
        { "DEBUG MOD", "Slobodno setanje, sva oruzja i tile-ovi (F2)", MenuChoice::Debug, true },
        { "SURVIVAL: ZOMBIJI", "Kosturi u talasima, svaki talas jaci. Koliko mozes da izdrzis?", MenuChoice::Survival, true },
        { "PRICA", "Uskoro...", MenuChoice::Story, false },
        { "IZLAZ", "Zatvori igru", MenuChoice::Quit, true },
    };
    m_selected = 1; // survival je podrazumevan
}

Rectangle MainMenu::buttonRect(int index, float uiScale) const
{
    float width = 140.0f * uiScale;
    float height = 18.0f * uiScale;
    float gap = 5.0f * uiScale;
    float top = GetScreenHeight() * 0.40f;
    return Rectangle{ (GetScreenWidth() - width) / 2.0f, top + index * (height + gap), width, height };
}

MenuChoice MainMenu::update(float dt, float uiScale)
{
    m_time += dt;
    if (m_lockedTimer > 0.0f)
        m_lockedTimer -= dt;

    int count = static_cast<int>(m_options.size());

    // tastatura: gore/dole (u krug)
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
        m_selected = (m_selected + count - 1) % count;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
        m_selected = (m_selected + 1) % count;

    // mis: dugme ispod misa postaje izabrano, ali samo kad se mis POMERI
    // (inace mis koji samo stoji iznad dugmeta ne bi dao da biras tastaturom)
    // (prvih 0.3 s ignorisemo: tada "pomeraj" postoji samo zato sto se prozor tek otvorio)
    Vector2 delta = GetMouseDelta();
    bool mouseMoved = (delta.x != 0.0f || delta.y != 0.0f) && m_time > 0.3f;
    bool clicked = false;
    Vector2 mouse = GetMousePosition();
    for (int i = 0; i < count; i++)
    {
        if (CheckCollisionPointRec(mouse, buttonRect(i, uiScale)))
        {
            bool click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
            if (mouseMoved || click)
                m_selected = i;
            if (click)
                clicked = true;
        }
    }

    bool confirm = clicked || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
    if (!confirm)
        return MenuChoice::None;

    const Option& option = m_options[m_selected];
    if (!option.enabled)
    {
        m_lockedTimer = 1.0f; // zatresi dugme
        return MenuChoice::None;
    }
    return option.choice;
}

void MainMenu::draw(float uiScale) const
{
    int scale = std::max(1, static_cast<int>(std::lround(uiScale * 2.0f / 3.0f)));
    int screenW = GetScreenWidth();

    // naslov sa senkom, lagano "pluta"
    const char* title = "MY RPG";
    int titleFont = 40 * scale;
    int titleW = MeasureText(title, titleFont);
    int titleY = static_cast<int>(GetScreenHeight() * 0.14f + std::sin(m_time * 1.5f) * 3.0f * scale);
    DrawText(title, (screenW - titleW) / 2 + 3 * scale, titleY + 3 * scale, titleFont, Fade(BLACK, 0.7f));
    DrawText(title, (screenW - titleW) / 2, titleY, titleFont, GOLD);

    const char* subtitle = "izaberi mod";
    int subFont = 10 * scale;
    DrawText(subtitle, (screenW - MeasureText(subtitle, subFont)) / 2, titleY + titleFont + 6 * scale, subFont,
             Fade(RAYWHITE, 0.8f));

    // dugmad
    int font = 10 * scale;
    for (int i = 0; i < static_cast<int>(m_options.size()); i++)
    {
        const Option& option = m_options[i];
        Rectangle r = buttonRect(i, uiScale);
        bool selected = (i == m_selected);

        // zakljucano dugme se trese kad ga kliknes
        if (selected && !option.enabled && m_lockedTimer > 0.0f)
            r.x += std::sin(m_lockedTimer * 60.0f) * 4.0f * m_lockedTimer;

        Color fill = selected ? Color{ 40, 36, 24, 230 } : Color{ 0, 0, 0, 170 };
        DrawRectangleRec(r, fill);
        if (selected)
            DrawRectangleLinesEx(r, std::max(2.0f, uiScale / 1.5f), option.enabled ? GOLD : GRAY);
        else
            DrawRectangleLinesEx(r, 1.0f, Fade(WHITE, 0.3f));

        Color textColor = option.enabled ? (selected ? GOLD : RAYWHITE) : GRAY;
        int textW = MeasureText(option.title, font);
        DrawText(option.title, static_cast<int>(r.x + (r.width - textW) / 2), static_cast<int>(r.y + (r.height - font) / 2),
                 font, textColor);
        if (!option.enabled)
        {
            const char* lock = "(uskoro)";
            int lockFont = 10;
            DrawText(lock, static_cast<int>(r.x + r.width) - MeasureText(lock, lockFont) - 6,
                     static_cast<int>(r.y + r.height) - lockFont - 4, lockFont, GRAY);
        }
    }

    // opis izabranog moda ispod dugmadi
    const Option& selected = m_options[m_selected];
    Rectangle last = buttonRect(static_cast<int>(m_options.size()) - 1, uiScale);
    int descY = static_cast<int>(last.y + last.height + 10 * uiScale);
    DrawText(selected.description, (screenW - MeasureText(selected.description, font)) / 2, descY, font,
             Fade(RAYWHITE, 0.85f));

    const char* help = "W/S ili strelice: biraj    Enter ili klik: potvrdi    F11: fullscreen";
    DrawText(help, (screenW - MeasureText(help, 10)) / 2, GetScreenHeight() - 20, 10, Fade(RAYWHITE, 0.6f));
}
