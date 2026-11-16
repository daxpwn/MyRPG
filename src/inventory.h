#pragma once
// Inventar sa 9 mesta. Ceo inventar JE hotbar (nema posebnog prozora).
//
// Svako mesto (Slot) cuva id oruzja iz weapons.cfg i koliko komada ima.
// Puske i noz su uvek po 1 komad, granate se slazu (maxStack u WeaponInfo).
// Biranje: tasteri 1-9, tockic misa ili klik na mesto.
//
// DroppedItems = stvari koje leze na zemlji (Q baca, prelazak preko pokupi).

#include <array>
#include <vector>
#include "raylib.h"
#include "weapons.h"

struct Slot
{
    int id = -1;   // -1 = prazno
    int count = 0;
};

class Inventory
{
public:
    static constexpr int SlotCount = 9;
    static constexpr int Empty = -1;

    Inventory();

    void clear();

    // Doda `count` komada. Prvo dopunjava postojece gomile istog oruzja,
    // pa koristi prazna mesta. Vraca koliko NIJE stalo (0 = sve je stalo).
    int add(int itemId, int count, const WeaponSet& weapons);

    // Koliko komada ove stvari jos moze da stane u inventar
    int spaceFor(int itemId, const WeaponSet& weapons) const;

    // Izvadi celo izabrano mesto (npr. za bacanje na zemlju)
    Slot takeSelected();
    // Potrosi jedan komad iz izabranog mesta (bacena granata)
    void consumeSelected();

    int selectedItem() const { return m_slots[m_selected].id; }
    int selectedSlot() const { return m_selected; }
    void select(int slot);

    // Tasteri 1-9, tockic, klik misem. Vraca true ako je mis iznad hotbara
    // (tada levi klik ne treba da puca).
    bool handleInput(float uiScale);

    // nameLift = koliko piksela iznad hotbara ide ime oruzja (da ne prekrije srca)
    void drawHotbar(const WeaponSet& weapons, float uiScale, float nameLift = 0.0f) const;
    // Pravougaonik celog hotbara (za HUD koji se crta iznad njega)
    Rectangle hotbarRect(float uiScale) const;

private:
    Rectangle slotRect(int slot, float uiScale) const;

    std::array<Slot, SlotCount> m_slots;
    int m_selected = 0;
    float m_nameTimer = 0.0f; // koliko jos dugo se vidi ime izabranog oruzja
};

// Stvari koje leze na mapi
class DroppedItems
{
public:
    void drop(int itemId, int count, Vector2 position);
    // Ako igrac (hitbox) dodiruje neku stvar i ima mesta u inventaru, pokupi je.
    void pickUp(Rectangle playerBox, Inventory& inventory, const WeaponSet& weapons);
    void update(float dt);
    void draw(const WeaponSet& weapons) const;
    void clear() { m_items.clear(); }

private:
    struct Item
    {
        int id = Inventory::Empty;
        int count = 1;
        Vector2 position{};       // centar, u pikselima sveta
        float pickupDelay = 0.0f; // da ne pokupis odmah ono sto si bacio
        float age = 0.0f;         // za animaciju lebdenja
    };
    std::vector<Item> m_items;
};
