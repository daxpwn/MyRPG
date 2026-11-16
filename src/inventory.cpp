#include "inventory.h"

#include <algorithm>
#include <cmath>

// ============================================================================
//  Inventory
// ============================================================================

Inventory::Inventory()
{
    clear();
}

void Inventory::clear()
{
    m_slots.fill(Slot{});
    m_selected = 0;
}

int Inventory::add(int itemId, int count, const WeaponSet& weapons)
{
    const WeaponInfo* info = weapons.find(itemId);
    if (info == nullptr)
        return count;
    int maxStack = info->maxStack;

    // 1) dopuni postojece gomile istog oruzja (npr. granate 2/5 -> 5/5)
    for (Slot& slot : m_slots)
    {
        if (count == 0)
            break;
        if (slot.id == itemId && slot.count < maxStack)
        {
            int space = maxStack - slot.count;
            int moved = std::min(space, count);
            slot.count += moved;
            count -= moved;
        }
    }

    // 2) ostatak na prazna mesta
    for (Slot& slot : m_slots)
    {
        if (count == 0)
            break;
        if (slot.id == Empty)
        {
            int moved = std::min(maxStack, count);
            slot.id = itemId;
            slot.count = moved;
            count -= moved;
        }
    }
    return count; // koliko nije stalo
}

int Inventory::spaceFor(int itemId, const WeaponSet& weapons) const
{
    const WeaponInfo* info = weapons.find(itemId);
    if (info == nullptr)
        return 0;
    int space = 0;
    for (const Slot& slot : m_slots)
    {
        if (slot.id == Empty)
            space += info->maxStack;
        else if (slot.id == itemId)
            space += info->maxStack - slot.count;
    }
    return space;
}

Slot Inventory::takeSelected()
{
    Slot taken = m_slots[m_selected];
    m_slots[m_selected] = Slot{};
    return taken;
}

void Inventory::consumeSelected()
{
    Slot& slot = m_slots[m_selected];
    slot.count--;
    if (slot.count <= 0)
        slot = Slot{};
}

void Inventory::select(int slot)
{
    if (slot < 0 || slot >= SlotCount)
        return;
    m_selected = slot;
    m_nameTimer = 1.5f;
}

bool Inventory::handleInput(float uiScale)
{
    // Tasteri 1-9: KEY_ONE, KEY_TWO... su uzastopni brojevi, pa moze petlja
    for (int i = 0; i < SlotCount; i++)
    {
        if (IsKeyPressed(KEY_ONE + i))
            select(i);
    }

    // Tockic: gore = prethodno mesto, dole = sledece (u krug)
    float wheel = GetMouseWheelMove();
    if (wheel > 0.0f)
        select((m_selected + SlotCount - 1) % SlotCount);
    else if (wheel < 0.0f)
        select((m_selected + 1) % SlotCount);

    m_nameTimer -= GetFrameTime();

    // Klik na mesto u hotbaru
    Vector2 mouse = GetMousePosition();
    for (int i = 0; i < SlotCount; i++)
    {
        if (CheckCollisionPointRec(mouse, slotRect(i, uiScale)))
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                select(i);
            return true; // mis je iznad hotbara
        }
    }
    return false;
}

Rectangle Inventory::slotRect(int slot, float uiScale) const
{
    float size = 20.0f * uiScale;
    float gap = 2.0f * uiScale;
    float total = SlotCount * size + (SlotCount - 1) * gap;
    float x0 = (GetScreenWidth() - total) / 2.0f;
    float y0 = GetScreenHeight() - size - 6.0f * uiScale;
    return Rectangle{ x0 + slot * (size + gap), y0, size, size };
}

Rectangle Inventory::hotbarRect(float uiScale) const
{
    Rectangle first = slotRect(0, uiScale);
    Rectangle last = slotRect(SlotCount - 1, uiScale);
    return Rectangle{ first.x, first.y, last.x + last.width - first.x, first.height };
}

void Inventory::drawHotbar(const WeaponSet& weapons, float uiScale, float nameLift) const
{
    // ugradjeni font je najostriji na 10, 20, 30...
    int fontSize = 10 * static_cast<int>(std::fmax(1.0f, std::round(uiScale * 2.0f / 3.0f)));
    int smallFontSize = 10;

    for (int i = 0; i < SlotCount; i++)
    {
        Rectangle r = slotRect(i, uiScale);
        bool selected = (i == m_selected);
        const Slot& slot = m_slots[i];

        DrawRectangleRec(r, Fade(BLACK, selected ? 0.75f : 0.5f));
        if (selected)
            DrawRectangleLinesEx(r, uiScale, GOLD);
        else
            DrawRectangleLinesEx(r, 1.0f, Fade(WHITE, 0.35f));

        if (slot.id != Empty)
        {
            // ikonica: 16x16 sprite uvecan uiScale puta, centriran u mestu
            float iconSize = 16.0f * uiScale;
            Rectangle icon{ r.x + (r.width - iconSize) / 2.0f, r.y + (r.height - iconSize) / 2.0f, iconSize, iconSize };
            weapons.drawIcon(slot.id, icon);

            // kolicina u donjem desnom uglu (samo za stvari koje se slazu)
            const WeaponInfo* info = weapons.find(slot.id);
            if (info != nullptr && info->maxStack > 1)
            {
                const char* text = TextFormat("x%d", slot.count);
                int w = MeasureText(text, smallFontSize);
                DrawText(text, static_cast<int>(r.x + r.width) - w - 3, static_cast<int>(r.y + r.height) - smallFontSize - 2,
                         smallFontSize, WHITE);
            }
        }

        // broj mesta (1-9) u gornjem levom uglu
        DrawText(TextFormat("%d", i + 1), static_cast<int>(r.x + uiScale + 2), static_cast<int>(r.y + uiScale + 1),
                 smallFontSize, selected ? GOLD : Fade(WHITE, 0.6f));
    }

    // ime izabranog oruzja iznad hotbara (polako nestaje)
    const WeaponInfo* w = weapons.find(m_slots[m_selected].id);
    if (w != nullptr && m_nameTimer > 0.0f)
    {
        float alpha = std::fmin(1.0f, m_nameTimer);
        int width = MeasureText(w->name.c_str(), fontSize);
        Rectangle first = slotRect(0, uiScale);
        DrawText(w->name.c_str(), (GetScreenWidth() - width) / 2, static_cast<int>(first.y - nameLift) - fontSize - 6,
                 fontSize, Fade(WHITE, alpha));
    }
}

// ============================================================================
//  DroppedItems
// ============================================================================

void DroppedItems::drop(int itemId, int count, Vector2 position)
{
    if (itemId == Inventory::Empty || count <= 0)
        return;
    Item item;
    item.id = itemId;
    item.count = count;
    item.position = position;
    item.pickupDelay = 1.0f; // sekunda pre nego sto moze ponovo da se pokupi
    m_items.push_back(item);
}

void DroppedItems::update(float dt)
{
    for (Item& item : m_items)
    {
        item.age += dt;
        if (item.pickupDelay > 0.0f)
            item.pickupDelay -= dt;
    }
}

void DroppedItems::pickUp(Rectangle playerBox, Inventory& inventory, const WeaponSet& weapons)
{
    for (size_t i = 0; i < m_items.size();)
    {
        Item& item = m_items[i];
        Rectangle box{ item.position.x - 6.0f, item.position.y - 6.0f, 12.0f, 12.0f };
        if (item.pickupDelay <= 0.0f && CheckCollisionRecs(playerBox, box))
        {
            item.count = inventory.add(item.id, item.count, weapons); // ostaje ono sto nije stalo
        }

        if (item.count <= 0)
        {
            // brisanje iz sredine vektora: zameni sa poslednjim pa izbaci poslednji (brze od erase)
            m_items[i] = m_items.back();
            m_items.pop_back();
        }
        else
        {
            i++;
        }
    }
}

void DroppedItems::draw(const WeaponSet& weapons) const
{
    for (const Item& item : m_items)
    {
        // lebdi gore-dole (sinusni talas) i ima senku ispod
        float bob = std::sin(item.age * 3.0f) * 1.5f;
        DrawEllipse(static_cast<int>(item.position.x), static_cast<int>(item.position.y + 6), 6.0f, 2.0f, Fade(BLACK, 0.3f));
        Rectangle dest{ std::round(item.position.x - 8.0f), std::round(item.position.y - 8.0f + bob), 16.0f, 16.0f };
        weapons.drawIcon(item.id, dest);
    }
}
