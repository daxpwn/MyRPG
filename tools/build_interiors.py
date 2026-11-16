#!/usr/bin/env python3
# ============================================================================
#  build_interiors.py
# ----------------------------------------------------------------------------
#  Uzme PRISTINI original atlas (assets/original/tiles_base.png) i doda ispod
#  njega tile-ove iz LimeZu "Modern Interiors" (free) seta. Originalni tile-ovi
#  0-16 ostaju netaknuti (stare mape rade).
#
#    1) atlas          -> assets/ufeff_tiles_v2.png
#    2) tiles.cfg      -> deo "MODERN TILES"
#    3) enterijer mape -> assets/interior_*.txt
#    4) ulazi          -> assets/map.txt
#
#  Pokretanje:  python3 tools/build_interiors.py   (treba Pillow)
#
#  Zidovi: blok ima GORNJI red (kapica + tijelo) i DONJI red (tijelo + lajsna),
#  pa sever(gornji) zid crtamo 2 reda visok -> izgleda kao pravi zid.
#  Podloga (base) namjestaja = pod, da providni coskovi pokazuju pod a ne crno.
#  Zato svi enterijeri koriste ISTI pod (pod_sivi) - kao i reference.
# ============================================================================
import os, json
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
ASSETS = os.path.join(ROOT, "assets")
MODERN = os.path.join(ASSETS, "original", "modern")
TS = 16
ATLAS_COLS = 16

ORIG_ATLAS = os.path.join(ASSETS, "ufeff_tiles_v2.png")
BASE_ATLAS = os.path.join(ASSETS, "original", "tiles_base.png")
RB = Image.open(os.path.join(MODERN, "Room_Builder_free_16x16.png")).convert("RGBA")
IN = Image.open(os.path.join(MODERN, "Interiors_free_16x16.png")).convert("RGBA")
SHEETS = {"RB": RB, "IN": IN}
FIRST_ID = 17

# --- 1x1 tile-ovi (podovi, zidovi, sitni dekor) -----------------------------
#   (sheet, col, row, ime, solid, base_ime)
SINGLES = [
    ("RB", 12, 11, "pod_sivi",     0, None),  # univerzalni pod
    ("RB", 12, 13, "pod_parket",   0, None),
    ("RB", 12,  7, "pod_zut",      0, None),
    ("RB",  1, 17, "zid_vrh",      1, None),  # plavo-sivi zid: kapica + tijelo
    ("RB",  1, 18, "zid",          1, None),  # plavo-sivi zid: tijelo + lajsna
    ("RB",  1, 19, "zid_tan_vrh",  1, None),  # topli (tan) zid
    ("RB",  1, 20, "zid_tan",      1, None),
    ("IN", 13, 36, "globus",       1, "pod_parket"),
    ("IN", 12, 45, "biljka_mala",  1, "pod_parket"),
    ("IN", 15, 28, "biljka",       1, "pod_parket"),
]
# --- visetilski komadi ------------------------------------------------------
#   (sheet, col, row, w, h, ime, solid, base_ime)
PIECES = [
    ("IN",  8,  8, 1, 2, "vrata_mod",    0, "pod_parket"),  # tamni prolaz = izlaz
    ("IN",  0,  1, 3, 3, "krevet",       1, "pod_parket"),
    ("IN",  8, 18, 2, 2, "sofa",         1, "pod_parket"),  # crvena sofa
    ("IN",  2, 21, 1, 2, "fotelja",      1, "pod_parket"),  # drvena fotelja
    ("IN",  4, 44, 1, 2, "fotelja_lj",   1, "pod_parket"),  # ljubicasta fotelja
    ("IN",  7, 21, 1, 2, "stolica",      0, "pod_parket"),
    ("IN",  2, 36, 2, 2, "sto",          1, "pod_parket"),  # radni sto
    ("IN",  2, 18, 2, 2, "polica",       1, "pod_parket"),  # polica s knjigama
    ("IN", 11, 24, 2, 2, "orman",        1, "pod_parket"),  # orman s ogledalom
    ("IN",  5, 26, 2, 2, "komoda",       1, "pod_parket"),
    ("IN",  8, 26, 2, 2, "pult",         1, "pod_parket"),  # kuhinjski pult
    ("IN", 11, 18, 2, 2, "tepih_z",      0, "pod_parket"),  # zeleni tepih
    ("IN",  8, 16, 2, 2, "tepih_c",      0, "pod_parket"),  # crveni tepih
    ("IN", 11, 44, 1, 2, "biljka_visoka",1, "pod_parket"),  # visoka biljka za cosak
    ("IN",  7, 24, 2, 2, "prozor",       1, "zid"),       # prozor na zidu
    ("IN", 11, 26, 1, 2, "slika",        1, "zid"),       # slika/ogledalo na zidu
    ("IN", 13, 40, 2, 2, "tabla",        1, "zid"),       # tabla na zidu
]

# ----------------------------------------------------------------------------
#  Rasporedi u atlasu
# ----------------------------------------------------------------------------
if not os.path.exists(BASE_ATLAS):
    Image.open(ORIG_ATLAS).convert("RGBA").save(BASE_ATLAS)
orig = Image.open(BASE_ATLAS).convert("RGBA")
orig_rows = orig.height // TS

placed = []           # (ime, atlas_col, atlas_row, solid, base, sheet, src_col, src_row)
pieces_grid = {}      # ime -> 2D lista imena sub-tile-ova

row, col = orig_rows, 0
for sh, sc, sr, name, solid, base in SINGLES:
    placed.append((name, col, row, solid, base, sh, sc, sr))
    col += 1
    if col >= ATLAS_COLS:
        col, row = 0, row + 1
row += 1; col = 0
for sh, sc, sr, w, h, name, solid, base in PIECES:
    grid = []
    for dy in range(h):
        line = []
        for dx in range(w):
            sub = f"{name}_{dy}_{dx}" if (w * h > 1) else name
            placed.append((sub, col + dx, row + dy, solid, base, sh, sc + dx, sr + dy))
            line.append(sub)
        grid.append(line)
    pieces_grid[name] = grid
    row += h

atlas = Image.new("RGBA", (ATLAS_COLS * TS, row * TS), (0, 0, 0, 0))
atlas.alpha_composite(orig, (0, 0))
for name, ac, ar, solid, base, sh, sc, sr in placed:
    atlas.alpha_composite(SHEETS[sh].crop((sc*TS, sr*TS, sc*TS+TS, sr*TS+TS)), (ac*TS, ar*TS))
atlas.save(ORIG_ATLAS)
print(f"atlas: {ATLAS_COLS}x{row} tile-ova -> {ORIG_ATLAS}")

name_to_id = {name: FIRST_ID + i for i, (name, *_1) in enumerate(placed)}

# ----------------------------------------------------------------------------
#  tiles.cfg
# ----------------------------------------------------------------------------
cfg_path = os.path.join(ASSETS, "tiles.cfg")
MARK = "# >>> MODERN TILES (generisano: tools/build_interiors.py) <<<"
text = open(cfg_path).read()
cut = text.find(MARK)
text = (text[:cut] if cut != -1 else text).rstrip() + "\n\n"
lines = [MARK, "# Ne diraj rucno - prepisuje se skriptom.",
         "# id  ime            kolona red solid podloga"]
for i, (name, ac, ar, solid, base, sh, sc, sr) in enumerate(placed):
    baseid = name_to_id.get(base, -1) if base else -1
    lines.append(f"{FIRST_ID+i:<5} {name:<14} {ac:<6} {ar:<3} {solid:<5} {baseid}")
open(cfg_path, "w").write(text + "\n".join(lines) + "\n")
print(f"tiles.cfg: dodato {len(placed)} tile-ova (id {FIRST_ID}..{FIRST_ID+len(placed)-1})")
json.dump({"name_to_id": name_to_id, "pieces": pieces_grid},
          open(os.path.join(HERE, "pieces.json"), "w"), indent=2)

# ----------------------------------------------------------------------------
#  Enterijer mape
# ----------------------------------------------------------------------------
def nid(name): return name_to_id[name]

class Room:
    def __init__(self, w, h, floor="pod_parket", wall="zid"):
        self.w, self.h, self.wall = w, h, wall
        self.g = [[nid(floor) for _ in range(w)] for _ in range(h)]
    def put(self, c, r, name):
        if 0 <= r < self.h and 0 <= c < self.w:
            self.g[r][c] = nid(name)
    def walls(self):
        vrh = self.wall + "_vrh"
        for c in range(self.w):
            self.put(c, 0, vrh)          # kapica
            self.put(c, 1, self.wall)    # tijelo (sever zid = 2 reda visok)
            self.put(c, self.h-1, self.wall)  # juzni zid
        for r in range(2, self.h-1):
            self.put(0, r, self.wall)
            self.put(self.w-1, r, self.wall)
    def stamp(self, c, r, piece):
        for dy, line in enumerate(pieces_grid[piece]):
            for dx, sub in enumerate(line):
                self.put(c+dx, r+dy, sub)
    def save(self, path, start, exit_cr, title):
        with open(path, "w") as f:
            f.write(f"# {title} - generisano: tools/build_interiors.py\n")
            f.write(f"# izlaz: stani na vrata dole da izadjes\n")
            f.write(f"start {start[0]} {start[1]}\n")
            f.write(f"izlaz {exit_cr[0]} {exit_cr[1]}\n\n")
            for grow in self.g:
                f.write(" ".join(f"{v:>2}" for v in grow) + "\n")
        print(f"mapa: {path}  ({self.w}x{self.h})")

# === DOM (stan): kompaktan i gusto namesten ================================
W, H = 16, 12                             # interijer: kol 1-14, red 2-10
dom = Room(W, H, floor="pod_parket", wall="zid")
dom.walls()
exd = (8, H-1)
dom.stamp(exd[0], H-2, "vrata_mod")
dom.put(exd[0], H-1, "vrata_mod_1_0")
# severni (dupli) zid: prozori + slike
for c in (1, 6, 11): dom.stamp(c, 0, "prozor")
for c in (4, 9, 14): dom.stamp(c, 0, "slika")
# --- gornji red uz zid: krevet, orman, polica, komoda, sofa, biljka ---
dom.stamp(1, 2, "krevet")                 # c1-3 r2-4
dom.stamp(4, 2, "orman")                  # c4-5 r2-3
dom.stamp(6, 2, "polica")                 # c6-7 r2-3
dom.stamp(8, 2, "komoda"); dom.put(10, 2, "globus")  # c8-9 r2-3
dom.stamp(11, 2, "sofa")                  # c11-12 r2-3
dom.stamp(14, 2, "biljka_visoka")         # cosak
# --- sredina: tepisi, sto sa stolicom, fotelje ---
dom.stamp(6, 5, "tepih_c")                # c6-7 r5-6
dom.stamp(9, 5, "sto"); dom.stamp(9, 7, "stolica")   # sto + stolica
dom.stamp(12, 5, "fotelja_lj"); dom.stamp(14, 5, "fotelja")
# --- donji red: police, komoda, tepih, biljke ---
dom.stamp(1, 6, "polica")                 # c1-2 r6-7
dom.stamp(4, 7, "komoda"); dom.put(6, 8, "globus")
dom.stamp(11, 8, "tepih_z")               # c11-12 r8-9
dom.stamp(1, 8, "biljka_visoka"); dom.stamp(14, 8, "biljka_visoka")
for c in (3, 8, 10): dom.put(c, 9, "biljka_mala")
dom.save(os.path.join(ASSETS, "interior_dom.txt"), start=(8, H-2), exit_cr=exd, title="DOM")

# === RADNJA/KLINIKA: kompaktna i gusta ======================================
W2, H2 = 14, 11                           # interijer: kol 1-12, red 2-9
kuh = Room(W2, H2, floor="pod_parket", wall="zid")
kuh.walls()
exk = (7, H2-1)
kuh.stamp(exk[0], H2-2, "vrata_mod")
kuh.put(exk[0], H2-1, "vrata_mod_1_0")
kuh.stamp(1, 0, "prozor"); kuh.stamp(8, 0, "prozor")
kuh.stamp(5, 0, "tabla"); kuh.stamp(11, 0, "slika")
# pultovi/ormani uz gornji zid
kuh.stamp(1, 2, "pult"); kuh.stamp(3, 2, "komoda"); kuh.stamp(5, 2, "pult")
kuh.stamp(11, 2, "orman"); kuh.put(8, 2, "biljka_mala")
# police sa robom
for c in (1, 4, 7): kuh.stamp(c, 5, "polica")
# radni kutak desno: sto + stolica + fotelja + tepih
kuh.stamp(10, 5, "sto"); kuh.stamp(10, 7, "stolica"); kuh.stamp(12, 5, "fotelja_lj")
kuh.stamp(2, 7, "tepih_c")
kuh.stamp(1, 8, "biljka_visoka"); kuh.stamp(12, 8, "biljka_visoka")
kuh.put(6, 8, "biljka_mala")
kuh.save(os.path.join(ASSETS, "interior_radnja.txt"), start=(7, H2-2), exit_cr=exk, title="RADNJA")

# ----------------------------------------------------------------------------
#  Ulazi u svetsku mapu (map.txt)
# ----------------------------------------------------------------------------
VRATA = 16
ENTRANCES = [(6, 6, "interior_dom.txt"), (23, 6, "interior_radnja.txt")]
map_path = os.path.join(ASSETS, "map.txt")
raw = open(map_path).readlines()
def is_grid(l):
    t = l.split(); return len(t) > 5 and all(x.lstrip("-").isdigit() for x in t)
header, grid = [], []
for ln in raw:
    if is_grid(ln): grid.append([int(x) for x in ln.split()])
    elif not ln.strip().startswith("ulaz"): header.append(ln.rstrip("\n"))
for c, r, fn in ENTRANCES:
    if 0 <= r < len(grid) and 0 <= c < len(grid[r]): grid[r][c] = VRATA
out, ins = [], False
for ln in header:
    out.append(ln)
    if ln.strip().startswith("start") and not ins:
        out += [f"ulaz {c} {r} {fn}" for c, r, fn in ENTRANCES]; ins = True
if not ins:
    out = [f"ulaz {c} {r} {fn}" for c, r, fn in ENTRANCES] + out
with open(map_path, "w") as f:
    f.write("\n".join(out) + "\n\n")
    for grow in grid: f.write(" ".join(f"{v:>2}" for v in grow) + "\n")
print(f"map.txt: dodato {len(ENTRANCES)} ulaza")
print("gotovo.")
