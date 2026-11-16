# Moja igrica (C++ / raylib)

## Struktura

```
game/
├── CMakeLists.txt        ← opis projekta: koji fajlovi, koje biblioteke
├── src/
│   ├── main.cpp          ← samo prozor i petlja (+ podsetnik raylib funkcija na dnu)
│   ├── game.h/.cpp       ← cela igra: ekrani (meni, igra, pauza, kraj), modovi, kamera, HUD
│   ├── menu.h/.cpp       ← glavni meni
│   ├── survival.h/.cpp   ← survival mod: tipovi protivnika, talasi, flow field, novcici
│   ├── shop.h/.cpp       ← prodavnica: soba, ponuda, kupovina
│   ├── tileset.h/.cpp    ← cita tiles.cfg, crta tile iz atlasa po id-u
│   ├── tilemap.h/.cpp    ← cita map.txt u 2D matricu id-eva, crta je, proverava sudare
│   ├── chunks.h/.cpp     ← deli mapu na chunkove 16x16, ucitava/izbacuje ih po kameri
│   ├── debugmap.h/.cpp   ← generise debug mapu sa svim tile-ovima (F2)
│   ├── player.h/.cpp     ← kretanje, sudari sa zidovima, animacija hodanja
│   ├── weapons.h/.cpp    ← cita weapons.cfg, crta oruzje (ikonica, u ruci, plamen)
│   ├── inventory.h/.cpp  ← inventar/hotbar sa 9 mesta + oruzje na zemlji
│   └── shooting.h/.cpp   ← borba: puske, noz, granate, eksplozije, cestice
├── assets/
│   ├── tiles.cfg         ← KOJI id je KOJI tile u atlasu (+ solid, podloga)
│   ├── map.txt           ← mapa debug moda: matrica id-eva
│   ├── arena.txt         ← arena za survival (+ "spawn" tacke = kapije)
│   ├── survival.cfg      ← tipovi protivnika i talasi: broj, zivot, brzina, novcici...
│   ├── shop.cfg          ← ponuda prodavnice: ime, cena, kolicina
│   ├── ui.png            ← HUD ikonice: srce, armor, novcic
│   ├── ufeff_tiles_v2.png← atlas tile-ova (16x16 polja)
│   ├── player.png        ← lik: 6 frejmova x 4 smera
│   ├── weapons.cfg       ← oruzja: steta, brzina, rasipanje, trzaj... (R = ucitaj ponovo)
│   ├── weapons.png       ← 15 pusaka + noz (redovi 0-1), plamen, granata, napitak, pancir, kaciga (red 2), minigun 4 frejma (redovi 3-10)
│   ├── icons.png         ← ikonice (jos se ne koriste)
│   └── original/         ← originalne slike pre obrade
└── build/                ← generisano, ne diraj (i ne commituj)
```

- **CMake** je alat koji od `CMakeLists.txt` napravi Visual Studio projekat.
- **raylib** je biblioteka koja ti daje prozor, crtanje, tastaturu, zvuk. CMake je sam skida.
- Spisak svih raylib funkcija: https://www.raylib.com/cheatsheet/cheatsheet.html

## Build i pokretanje (u terminalu, iz `game` foldera)

Prvi put, ili kad promeniš `CMakeLists.txt`:

```bash
cmake -S . -B build
```

Svaki put kad promeniš kod:

```bash
cmake --build build --config Debug
```

Pokretanje (može iz bilo kog foldera, putanja do `assets/` je ugrađena u exe):

```bash
.\build\Debug\game.exe
```

Ako više voliš Visual Studio: otvori `build\game.sln`, desni klik na `game` →
*Set as Startup Project*, pa F5 (tako možeš i da debuguješ, sa breakpoint-ima).

## Kontrole

| Taster          | Šta radi                                                  |
|-----------------|-----------------------------------------------------------|
| Strelice / WASD | kretanje                                                  |
| Miš             | nišan (lik se okreće ka mišu)                             |
| Levi klik       | napad: pucaj / zamah nožem / baci granatu                 |
| 1-9 / točkić    | biranje mesta u hotbaru (ili klik na mesto)               |
| Q               | baci izabrano oružje (prelaskom preko njega ga pokupiš)   |
| E               | kupi stvar ispred koje stojiš (u prodavnici)              |
| F11 / Alt+Enter | fullscreen (borderless) i nazad                           |
| F2              | (debug mod) mapa sa SVIM tile-ovima i oružjima            |
| F1              | debug: FPS, pozicija, ivice chunkova (žuto), hitbox       |
| F3              | chunkovi uključeni/isključeni (za poređenje)              |
| R               | ponovo učita sve `.cfg` fajlove i mape                     |
| Esc             | pauza (u pauzi: Esc = nastavi, M = glavni meni)           |

Prozor može i da se razvlači mišem. Zoom se sam bira po visini prozora
(600px → 3x, 1080px → 5x) i uvek je ceo broj, da pixel art ostane oštar.

## Modovi (glavni meni)

- **Debug mod**: mapa iz `map.txt`, sva oružja, F2 = mapa sa svim tile-ovima i oružjima.
- **Survival: zombiji**: arena iz `arena.txt`. Protivnici izlaze iz 4 kapije u talasima.
  - **3 tipa** (`neprijatelj` redovi u `survival.cfg`): *kostur* (obican), *zombi*
    (brz, slab, od 2. talasa), *sluz* (spora, jaka, jace udara, od 3. talasa),
  - svaki talas: više protivnika, više života, brži (formula je na vrhu `survival.cfg`),
  - zaobilaze stubove: **flow field** (BFS od igrača daje svakom polju broj
    koraka do tebe, a protivnik ide ka susedu sa manjim brojem),
  - kad ih ubiješ ispadne **1-3 novčića** (privuku se kad priđeš), a 10% šanse i oružje,
  - posle talasa: +2 srca i 12 s pauze; tada se u areni pojave **vrata prodavnice**,
  - počinješ sa Glock 17, nožem i 3 granate. Kad padne život na 0: Enter = ponovo.
- **Prodavnica** (`shop.cfg`): uđeš kroz vrata u pauzi, talas čeka dok si unutra.
  Stani na stvar, dole piše cena, **E** kupuje. Ima sve: napitak, pancir, kacigu,
  granate, nož i sve puške. Izlaz su vrata na dnu sobe.
- **Priča**: uskoro.

## Srca i armor

- Iznad hotbara: **10 srca** (1 srce = 10 života, pola srca = 5) i iznad njih **10 mesta za
  armor**. Desno su novčići. Kad imaš 2 srca ili manje, srca drhte.
- **Pancir** daje 5 armora, **kaciga** 5. Svaki poen armora smanjuje štetu 6%
  (pun armor = 60% manje štete). Armor traje do kraja igre.
- **Heal potion** vraća 4 srca: izaberi ga u hotbaru i levi klik.

## Oružje i inventar

- **Inventar = hotbar**: 9 mesta, svako čuva id oružja iz `weapons.cfg` i količinu.
  Granate se slažu do 5 u jednom mestu (`x3` u uglu).
- **Nož**: zamah u luku ispred tebe (beli trag), pogađa sve u luku. U `weapons.cfg`:
  `rasipanje` = pola luka, `domet` = dohvat.
- **Granata**: leti u luku do miša (najdalje `metak` piksela), odskoči, kotrlja se,
  trepće i posle 1.4 s eksplodira (`domet` = poluprečnik, manja šteta na ivici).
- **15 pušaka + nož + granata** u `weapons.cfg`: Glock 17, Desert Eagle, Colt Python, UZI, Thompson, MP40,
  AK-47, M4, M16, SCAR, Kar98k, M1 Garand, AWP, M60, Remington 870. Sva leže na podu
  debug mape (F2). Ime sa razmakom se u cfg piše sa `_` (`Desert_Eagle`).
- **Minigun**: najjača puška (16 štete, 30 metaka u sekundi). Kad držiš klik, cevi se
  prvo zavrte (0.35 s), pa tek onda puca. Dok se vrte, cevi se animiraju: 4 frejma
  u `weapons.png` (redovi 3-10, jedan ispod drugog), u kojima su redovi piksela cevi
  pomereni za 2. Kad pustiš klik, cevi polako stanu. Slika je velika (93x27 piksela,
  polje 6x2) i crta se na 40% (`skala` u `weapons.cfg`), pa pikseli ostaju oštri.
  Ima ga u debug modu (mesto 8) i u prodavnici za 150 novčića.
- **Pucanje** (`shooting.cpp`), sve što se vidi kad opališ:
  - *trzaj*: oružje se trzne unazad i cev se podigne, pa se vrati,
  - *plamen iz cevi*: 3 frejma iz `weapons.png` (red 1), traje 0.06 s,
  - *metak*: svetla linija sa repom; staje na solid tile-u i pravi varnice,
  - *čaura*: izleti sa strane oružja i uspori po zemlji,
  - *trešenje ekrana*: jače za oružje sa većim trzajem.
- Oružje se okreće oko **drške** (`drska_x/y`), a metak izlazi iz **cevi** (`cev_x/y`).
  Kad nišaniš levo, sprite se obrne po vertikali da oružje ne bi bilo naopako.
- Sve brojke (šteta, brzina pucanja, rasipanje, broj sačmi, trzaj, domet) su u
  `weapons.cfg`. Promeni, pritisni **R** i odmah probaj.

Sprite-ovi oružja su privremeni (nacrtani za ovu igru). Zameniš ih tako što
promeniš `weapons.png` i u `weapons.cfg` podesiš kolonu/red, dršku i cev.

## Chunkovi

Mapa se deli na kvadrate od 16x16 tile-ova (`chunks.h`). Svaki frejm:

1. izračuna se koje chunkove kamera vidi (+1 chunk rezerve okolo),
2. oni koji fale se **učitaju**: kopiraju se njihovi id-evi i jednom se nacrtaju u
   sopstvenu teksturu (`RenderTexture2D`),
3. oni koji su daleko se **izbace** (tekstura se oslobodi).

Posle toga se chunk crta kao jedna slika umesto 256+ tile-ova. F1 pokazuje ivice i
broj učitanih chunkova. Podaci mape su i dalje celi u memoriji (mali su), a za ogroman
svet bi se samo `ChunkManager::loadChunk` promenio da čita chunk sa diska.

Kad se tile u mapi promeni dok igra radi, njegov chunk mora ponovo da se "ispeče"
(zato R i F2 zovu `chunks.clear()`).

## Debug mapa (F2)

Pravi se iz koda (`debugmap.cpp`), ne iz fajla:
- gore: **sva oružja** iz `weapons.cfg` na podu, sa imenom (prođi preko = pokupi),
- sredina: svaki tile iz `tiles.cfg` sa natpisom `id ime` (`[S]` = solid),
- dole: **ceo atlas**, svako polje sa `kolona,red` — to prepišeš u `tiles.cfg`.
  Crno iza polja = providni delovi slike.

## Kako menjaš mapu

1. Otvori `assets/map.txt`, promeni brojeve (id-eve iz `tiles.cfg`), sačuvaj.
2. U igri pritisni **R**. Nema builda.

Ako nešto ne valja (redovi različite dužine, slovo umesto broja), igra zadrži staru
mapu i napiše grešku u konzoli. Nepoznat id se crta kao roze-crni kvadrat.

## Kako dodaš novi tile

1. Nađi ga u `ufeff_tiles_v2.png` i izbroj kolonu i red (od 0, svako polje je 16x16).
2. Dodaj red na dno tabele u `tiles.cfg` sa sledećim slobodnim id-em.
3. Ako tile ima providnu pozadinu, upiši `podloga` (npr. 0 = trava ispod).
4. Koristi novi id u `map.txt`, pa **R**.

## Vežbe (redom, svaka uči nešto novo)

1. Dodaj nove tile-ove u `tiles.cfg` (npr. vrata, merdevine) i stavi ih na mapu.
2. Promeni brzinu igrača i zoom kamere. Gde su te vrednosti?
3. Dodaj trčanje: dok držiš Shift, brzina je duplo veća (i animacija brža).
4. Kad igrač stane, neka gleda u smer u kom je poslednji išao (već radi, pronađi zašto).
5. Napravi veliku mapu (npr. 200x200) i uporedi F3 uključeno/isključeno. Da bi video
   razliku, privremeno isključi `SetTargetFPS(60)` i `FLAG_VSYNC_HINT`.
6. Dodaj drugi sloj mape (npr. `map_objekti.txt`) za stvari preko zemlje, umesto `podloga` kolone.
7. Iskoristi `icons.png`: srce u ćošku ekrana (UI) i novčić na mapi koji se pokupi kad ga dodirneš.
8. Napravi NPC-a: još jedan `Player`-sličan objekat koji se šeta levo-desno.
9. Dodaj municiju: svako oružje ima šaržer (`weapons.cfg`), broj metaka piše u hotbaru,
   a na prazan šaržer se puni tasterom (koji je slobodan? R je zauzet).
10. Napravi metu (npr. bure) koja ima život i nestane kad je pogodiš dovoljno puta (`damage`).
