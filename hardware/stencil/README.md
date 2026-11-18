# Solder paste stencil

Stencil files for every SMD pad on `NG_WPI_7seg_PCB` (the clock board), generated straight from the KiCad file by `build_stencil.py`.

- **Top:** 207 openings (31 SK6812MINI LEDs, ATmega32U4 TQFP-44, USB micro-B, crystal, C1, 0805 parts)
- **Bottom:** 12 openings (the three PTS647 buttons)

The pad positions were checked against the paste Gerbers that went to JLCPCB (`../pcb/fabrication/fab files sent to jlc/`): all 219 match.

## Which file to use

| You want to | Use | Notes |
|---|---|---|
| **Order a steel stencil (recommended)** | `out/jlc/clock_stencil_gerbers.zip` | JLCPCB: *SMT-Stencil*, upload the zip, frameless, **0.12 mm** thickness, top + bottom on one stencil (or top only if you hand solder the buttons). |
| Laser cut Mylar / Kapton (0.10 to 0.125 mm) | `out/laser/stencil_top.svg` / `.dxf` | 1:1 in mm. Red = cut, blue dashed = board outline (reference only, don't cut). The 0.32 mm QFP and USB slots are near the limit of a CO2 laser. |
| Resin print | `out/print/stencil_top_resin.stl` | 0.20 mm plate with a lip that drops over the board edge. Print lip up, flip to use. FDM printers can't make the 0.3 mm slots. |
| Hold the board while printing paste | `out/print/board_jig.stl` | Nest with a 1.6 mm pocket so the board sits flush with the jig top, plus pockets for the back-side buttons if they're already on. Tape the steel stencil to the jig. |

Bottom files are mirrored (as seen when you look at the back of the board).

## Aperture reductions

Openings are slightly smaller than the copper pads to stop bridging and solder balls:

| Part | Copper pad | Opening | Why |
|---|---|---|---|
| ATmega32U4 TQFP-44 (0.8 mm pitch) | 0.55 x 1.50 | 0.48 x 1.45 | Fine pitch, the usual bridge spot |
| USB micro-B signal pins (0.65 mm pitch) | 0.40 x 1.40 | 0.32 x 1.30 | Finest pitch on the board |
| Crystal, USB shell tabs | 2.0 x 2.4 / 1.75 x 1.9 | -0.10 mm per side | Large pads, less paste avoids floating |
| Everything else (LEDs, 0805, C1, buttons) | | -0.05 mm per side | ~10% less paste, standard |

The thinnest web of steel left between two openings is 0.32 mm (QFP and USB).

## Rebuilding

```
pip install sexpdata shapely ezdxf matplotlib manifold3d
python3 hardware/stencil/build_stencil.py
```

Change the rules in `aperture_size()` if you want different openings.
