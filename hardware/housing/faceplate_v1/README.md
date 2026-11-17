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
