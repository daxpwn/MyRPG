#pragma once
// Glavni meni: lista dugmadi, biranje strelicama/W-S ili misem, Enter/klik potvrdjuje.

#include <vector>
#include "raylib.h"

enum class MenuChoice
{
    None,     // nista nije izabrano u ovom frejmu
    Debug,
    Survival,
    Story,
    Quit,
};

class MainMenu
{
public:
    MainMenu();

    // Cita tastaturu i mis. Vraca izbor kad igrac potvrdi (inace None).
    MenuChoice update(float dt, float uiScale);
    void draw(float uiScale) const;

private:
    struct Option
    {
        const char* title;
        const char* description;
        MenuChoice choice;
        bool enabled; // story jos ne postoji
    };

    Rectangle buttonRect(int index, float uiScale) const;

    std::vector<Option> m_options;
    int m_selected = 0;
    float m_time = 0.0f;
    float m_lockedTimer = 0.0f; // kratko "uskoro!" kad kliknes na zakljucan mod
};
