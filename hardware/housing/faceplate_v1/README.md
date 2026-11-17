# Face plate v1 + open-back shell

Built by `build_faceplate.py` (CadQuery) from `PCB_CLOCK_digit_v1_source.step`.
The digit windows and light-guide walls are cropped out of that file unchanged; everything else is new.

    pip install cadquery
    python build_faceplate.py PCB_CLOCK_digit_v1_source.step .

- `clock_front_plate` – 145.2 x 66.2 mm, 1.2 mm face, print face down. Five bosses for M3 heat-set inserts line up with the board's five 3.2 mm holes; four taller bosses take the shell screws.
- `clock_shell` – 3 mm wall ring, 30 mm deep, open back. Print back edge down, no supports.
- Inserts: M3, 4.0 mm hole (OD 4.2–4.6 mm), max 4 mm long in the board bosses, max 6 mm in the shell bosses.
- Screws: M3x4 or M3x5 for the board, M3x6 or M3x8 for the shell.
- Coordinates match the Fusion file. Board corner (0,0) is at X 9.32, Y 76.75; the board front sits at z = 5.

## Scroll knob (rotary encoder) — added 2026-11-17

- **Encoder:** KY-040 module (EC11-type encoder, M7 bushing, 6 mm D-shaft, measured 5.88 mm round / 4.37 mm across the flat).
- **Shell:** 7.5 mm hole through the **top wall**, centred at X −85 (above the right-hand digit as seen from the front) and halfway back (z 16.2). From the inside, a 13 × 13 mm square pocket thins the wall to 1.5 mm so the encoder body sits in it (stops it rotating) and the bushing sticks out far enough for the nut. Mount it from inside: encoder body into the pocket, washer + nut on top.
- **Knob (`clock_knob`):** 20 mm wide, 16 mm tall, 12 gentle waves (±0.35 mm) with a 12° twist, chamfered top with a pointer groove. Underside has a 12.5 mm × 4 mm recess for the nut and bushing end; D bore 6.1 mm round / 4.5 mm flat-to-round, 13.8 mm deep. Print it standing on its open underside (groove up), no supports. If it is loose, print with 0.1 mm less in the bore (`6.1` / `4.5` in the script); if too tight, ream with a 6 mm drill.
- The encoder module hangs about 10 mm down from the top wall, clear of the board and the shell bosses.
