// Clauck housing: parametric desk-clock enclosure for NG_WPI_7seg_PCB
// ---------------------------------------------------------------------
// OpenSCAD 2021+.  Pick a part with the `part` variable (or -D part="...")
//   "assembly"  everything stacked, for looking at
//   "front"     front shell with the light-guide grid (print face down)
//   "back"      back plate with clamp posts, vents and the RTC cradle
//   "plunger"   one button plunger (print 3, standing on its cap)
//   "section"   assembly cut through SW1 and digit 1, to check fit
//
// Coordinates: board frame is KiCad's, origin at the board's top-left
// corner, x right, y DOWN, all mm. Everything below is derived from
// hardware/pcb/source (kicad files)/NG_WPI_7seg_PCB.kicad_pcb.
// In the model, Z = 0 is the PCB's front copper face, +Z is toward the
// viewer, and the case's Y axis points UP (y_case = BOARD_H - y_board).

part = "assembly";

/* ---------------- board facts (from the .kicad_pcb) ---------------- */
BOARD_W = 102.0;
BOARD_H = 40.0;
PCB_T   = 1.6;
HOLE    = [86.0, 4.0];          // the only M3 hole
USB_Y   = 15.315;               // J4 centre, exits the LEFT edge, front side
BTN_X   = [44.375, 53.0, 61.625]; // SW1..SW3 on the BACK, top-actuated
BTN_Y   = 37.05;
BTN_H   = 5.0;                  // PTS647 Sx50 total height above PCB back

// SK6812MINI positions: [ref, x, y, rot]. rot 0/180 = horizontal segment,
// +-90 = vertical segment. D17/D18 are the colon, D33 the decimal point.
LEDS = [
  ["D3", 39.1, 31.5, 0],   ["D4", 33.625, 25.25, -90], ["D5", 44.625, 25.25, -90],
  ["D6", 39.1, 19.0, 0],   ["D7", 33.6, 12.5, -90],    ["D8", 44.6, 12.5, -90],
  ["D9", 39.1, 6.325, 0],
  ["D10", 56.1, 6.275, 180], ["D11", 50.6, 12.5, 90],  ["D12", 61.6, 12.5, 90],
  ["D13", 61.625, 25.25, 90],["D14", 56.1, 31.5, 0],   ["D15", 50.625, 25.25, -90],
  ["D16", 56.1, 19.0, 180],
  ["D17", 66.625, 14.15, 90], ["D18", 66.625, 23.7, 90], ["D33", 66.625, 31.35, 90],
  ["D19", 71.6, 12.5, -90],  ["D20", 77.1, 6.325, 180], ["D21", 82.6, 12.5, 90],
  ["D22", 77.1, 19.125, 0],  ["D23", 71.625, 25.25, 90],["D24", 77.1, 31.5, 180],
  ["D25", 82.575, 25.25, -90],
  ["D26", 88.625, 25.25, -90],["D27", 88.575, 12.5, -90],["D28", 94.05, 6.325, 180],
  ["D29", 99.6, 12.55, 90],  ["D30", 94.1, 19.0, 180], ["D31", 99.625, 25.25, 90],
  ["D32", 94.1, 31.5, 0],
];
DOTS = ["D17", "D18", "D33"];

// Front-side electronics (MCU, crystal, C1 electrolytic 5.3 mm tall,
// header pads). Hollowed out behind a solid bezel.
ELEC_ZONE = [0.6, 30.6];        // x range on the board

/* ---------------- tunables ---------------- */
MARGIN     = 8.0;   // bezel around the board on every side
WALL       = 2.4;   // side walls
CORNER_R   = 3.0;   // outer corner radius
FACE_T     = 1.2;   // front wall over the electronics zone
GUIDE_D    = 7.0;   // light-guide depth, PCB face to front face (more = softer)
SEG_W      = 4.0;   // segment cell width (LED body is 3.5)
SEG_GAP    = 0.8;   // tip-to-corner gap -> ~1.1 mm wall between segments
DOT_D      = 4.2;   // colon / decimal cell diameter
BACK_DEPTH = 14.0;  // PCB back face to back-plate inner face (fits DS3231)
BACK_T     = 2.5;   // back plate thickness
CLR        = 0.3;   // fit clearance

// "inset": segment cells are open, and the front has a pocket for a
//          diffuser / smoked sheet (DIFF_T thick) glued in flush.
// "skin":  cells are closed by a SKIN_T skin. Print the first layers in
//          white/natural and switch to black for a two-colour face.
// "open":  bare segment holes, no diffuser.
FACE_MODE  = "inset";
DIFF_T     = 1.0;
SKIN_T     = 0.6;
DIFF_PAD   = 3.0;   // diffuser sheet overhang around the display

USB_W = 12.0; USB_H = 8.0; USB_Z = 1.4;   // tunnel for the plug overmold

SCREW_D   = 2.6;    // M3 self-tapping into the boss (use 4.0 for heat-set)
SCREW_CLR = 3.4;
SCREW_HEAD= 6.0;
BOSS_D    = 6.4;

PLUNGER_D = 4.0;    // through the back plate
FLANGE_D  = 7.0;
FLANGE_T  = 1.2;
CAP_OUT   = 2.5;    // how far the cap stands proud of the back

// hiLetgo DS3231 (ZS-042) module, lying battery-down on the back plate
RTC_SIZE  = [38.5, 22.5];
RTC_POS   = [74.0, 18.0];      // centre, board coords
RTC_LIP   = 3.0;

// Back plate posts that press the PCB against the light-guide grid
CLAMP_POSTS = [[4, 4], [4, 36], [99.5, 3], [98, 36], [30, 20], [56, 3]];

$fn = 48;

/* ---------------- derived ---------------- */
OUT_W   = BOARD_W + 2 * MARGIN;
OUT_H   = BOARD_H + 2 * MARGIN;
Z_FRONT = GUIDE_D;                         // front face of the case
Z_BACK  = -(PCB_T + BACK_DEPTH);           // shell bottom = plate inner face
BOSS_C  = WALL + BOSS_D / 2 - 0.6;         // boss centre from the outer edge

function bx(x) = x + MARGIN;               // board -> case X
function by(y) = BOARD_H - y + MARGIN;     // board -> case Y (flipped)
function is_dot(r) = len([for (d = DOTS) if (d == r) 1]) > 0;

/* ---------------- 2D helpers ---------------- */
module rrect(w, h, r) {
  hull() for (x = [r, w - r], y = [r, h - r]) translate([x, y]) circle(r);
}

// Hexagonal 7-seg cell, long axis along X, half length a.
module seg_hex(a, w) {
  polygon([[-a, 0], [-a + w / 2, -w / 2], [a - w / 2, -w / 2],
           [a, 0], [a - w / 2, w / 2], [-a + w / 2, w / 2]]);
}

// Horizontal segments meet verticals at a corner 5.5 mm across and
// ~6.2 mm down from their centres, so their tips stop SEG_GAP short.
module led_cells_2d() {
  for (l = LEDS) translate([bx(l[1]), by(l[2])]) {
    if (is_dot(l[0])) circle(d = DOT_D);
    else if (l[3] == 0 || l[3] == 180) seg_hex(5.5 - SEG_GAP, SEG_W);
    else rotate(90) seg_hex(6.2 - SEG_GAP, SEG_W);
  }
}

module display_rect_2d(pad) {
  x0 = 33.6 - SEG_W / 2 - pad; x1 = 99.6 + SEG_W / 2 + pad;
  y0 = 6.3 - 5.5 / 2 - pad;    y1 = 31.5 + 5.5 / 2 + pad;
  translate([bx(x0), by(y1)]) square([x1 - x0, y1 - y0]);
}

/* ---------------- front shell ---------------- */
module front_shell() {
  difference() {
    union() {
      difference() {
        translate([0, 0, Z_BACK]) linear_extrude(Z_FRONT - Z_BACK) rrect(OUT_W, OUT_H, CORNER_R);
        // everything behind the PCB face is hollow
        translate([WALL, WALL, Z_BACK - 1])
          cube([OUT_W - 2 * WALL, OUT_H - 2 * WALL, -Z_BACK + 1]);
      }
      // corner screw bosses
      for (x = [BOSS_C, OUT_W - BOSS_C], y = [BOSS_C, OUT_H - BOSS_C])
        translate([x, y, Z_BACK]) cylinder(d = BOSS_D, h = -Z_BACK);
      // locating rim around the board edge (PCB-thick)
      translate([0, 0, -PCB_T]) linear_extrude(PCB_T) difference() {
        translate([MARGIN - CLR - 1.2, MARGIN - CLR - 1.2])
          square([BOARD_W + 2 * CLR + 2.4, BOARD_H + 2 * CLR + 2.4]);
        translate([MARGIN - CLR, MARGIN - CLR])
          square([BOARD_W + 2 * CLR, BOARD_H + 2 * CLR]);
      }
      // pin through the board's M3 hole
      translate([bx(HOLE[0]), by(HOLE[1]), -PCB_T]) cylinder(d = 2.9, h = PCB_T + 0.01);
    }
    // segment light guides
    cell_top = FACE_MODE == "skin" ? Z_FRONT - SKIN_T : Z_FRONT + 1;
    translate([0, 0, -0.01]) linear_extrude(cell_top + 0.01) led_cells_2d();
    // diffuser pocket
    if (FACE_MODE == "inset")
      translate([0, 0, Z_FRONT - DIFF_T]) linear_extrude(DIFF_T + 1) display_rect_2d(DIFF_PAD);
    // electronics pocket behind the left bezel
    translate([bx(ELEC_ZONE[0]), by(BOARD_H - 0.6), -0.01])
      cube([ELEC_ZONE[1] - ELEC_ZONE[0], BOARD_H - 1.2, Z_FRONT - FACE_T + 0.01]);
    // USB tunnel through the left wall
    translate([-1, by(USB_Y) - USB_W / 2, USB_Z - USB_H / 2])
      cube([MARGIN + 1.5, USB_W, USB_H]);
    // screw pilot holes
    for (x = [BOSS_C, OUT_W - BOSS_C], y = [BOSS_C, OUT_H - BOSS_C])
      translate([x, y, Z_BACK - 1]) cylinder(d = SCREW_D, h = -Z_BACK - PCB_T);
  }
}

/* ---------------- back plate ---------------- */
module vent_slots() {
  for (i = [0 : 5]) translate([bx(70 + i * 5.0), by(1.5) - 0.5, 0]) square([2.2, 5]);
  for (i = [0 : 5]) translate([bx(70 + i * 5.0), by(38.5) - 4.5, 0]) square([2.2, 5]);
}

module back_plate() {
  difference() {
    union() {
      translate([0, 0, Z_BACK - BACK_T]) linear_extrude(BACK_T) rrect(OUT_W, OUT_H, CORNER_R);
      // spigot that keys into the shell opening
      translate([WALL + CLR, WALL + CLR, Z_BACK]) linear_extrude(1.5) difference() {
        square([OUT_W - 2 * (WALL + CLR), OUT_H - 2 * (WALL + CLR)]);
        offset(-1.2) square([OUT_W - 2 * (WALL + CLR), OUT_H - 2 * (WALL + CLR)]);
        for (x = [BOSS_C, OUT_W - BOSS_C], y = [BOSS_C, OUT_H - BOSS_C])
          translate([x - WALL - CLR, y - WALL - CLR]) circle(d = BOSS_D + 2 * CLR);
      }
      // clamp posts, stop 0.1 short so the board is snug, not bent
      for (p = CLAMP_POSTS) translate([bx(p[0]), by(p[1]), Z_BACK])
        cylinder(d = 4, h = BACK_DEPTH - 0.1);
      // DS3231 cradle (corner lips; glue or zip-tie the module in)
      translate([bx(RTC_POS[0]), by(RTC_POS[1]), Z_BACK]) linear_extrude(RTC_LIP)
        difference() {
          square([RTC_SIZE[0] + 3, RTC_SIZE[1] + 3], center = true);
          square([RTC_SIZE[0], RTC_SIZE[1]], center = true);
          square([RTC_SIZE[0] - 12, RTC_SIZE[1] + 4], center = true);
          square([RTC_SIZE[0] + 4, RTC_SIZE[1] - 10], center = true);
        }
      // plunger guide sleeves
      for (x = BTN_X) translate([bx(x), by(BTN_Y), Z_BACK])
        cylinder(d = PLUNGER_D + 3, h = 2);
    }
    // screws (countersunk from the back)
    for (x = [BOSS_C, OUT_W - BOSS_C], y = [BOSS_C, OUT_H - BOSS_C]) {
      translate([x, y, Z_BACK - BACK_T - 1]) cylinder(d = SCREW_CLR, h = BACK_T + 5);
      translate([x, y, Z_BACK - BACK_T - 0.01]) cylinder(d1 = SCREW_HEAD, d2 = SCREW_CLR, h = 1.6);
    }
    for (x = BTN_X) translate([bx(x), by(BTN_Y), Z_BACK - BACK_T - 1])
      cylinder(d = PLUNGER_D + 2 * CLR, h = BACK_T + 4);
    translate([0, 0, Z_BACK - BACK_T - 1]) linear_extrude(BACK_T + 2) vent_slots();
    // RTC wire / zip-tie slots
    for (s = [-1, 1]) translate([bx(RTC_POS[0]) + s * 8, by(RTC_POS[1]), Z_BACK - BACK_T - 1])
      linear_extrude(BACK_T + 2) square([1.6, 5], center = true);
  }
}

/* ---------------- plunger ---------------- */
// Length from the plate's inner face to the switch actuator, minus 0.2.
PLUNGER_IN = BACK_DEPTH - BTN_H - 0.2;
module plunger() {
  // origin = back plate inner face, +Z toward the PCB
  cylinder(d = PLUNGER_D, h = PLUNGER_IN);              // stem to the switch
  cylinder(d = FLANGE_D, h = FLANGE_T);                 // retaining flange
  translate([0, 0, -BACK_T - CAP_OUT])                  // button cap outside
    cylinder(d = PLUNGER_D, h = BACK_T + CAP_OUT);
}

/* ---------------- reference PCB ---------------- */
module pcb_ghost() {
  color("darkgreen") translate([MARGIN, MARGIN, -PCB_T]) difference() {
    cube([BOARD_W, BOARD_H, PCB_T]);
    translate([HOLE[0], BOARD_H - HOLE[1], -1]) cylinder(d = 3.2, h = 3);
  }
  color("white") for (l = LEDS) translate([bx(l[1]), by(l[2]), 0.5]) cube([3.5, 3.5, 1], center = true);
  color("silver") translate([MARGIN - 0.6, by(USB_Y) - 3.9, 0]) cube([5, 7.8, 2.8]);
  color("black") for (x = BTN_X) translate([bx(x), by(BTN_Y), -PCB_T - BTN_H / 2])
    cube([4.5, 4.5, BTN_H], center = true);
  color("royalblue") translate([bx(RTC_POS[0]), by(RTC_POS[1]), Z_BACK + 5])
    cube([38, 22, 10], center = true);
}

module plungers_in_place() {
  for (x = BTN_X) translate([bx(x), by(BTN_Y), Z_BACK]) plunger();
}

module assembly() {
  color("dimgray", 0.9) front_shell();
  color("gray") back_plate();
  color("orange") plungers_in_place();
  pcb_ghost();
}

/* ---------------- part selector ---------------- */
if (part == "assembly") assembly();
else if (part == "front") translate([0, 0, Z_FRONT]) mirror([0, 0, 1]) front_shell();
else if (part == "back") translate([0, 0, -(Z_BACK - BACK_T)]) back_plate();
else if (part == "plunger") translate([0, 0, BACK_T + CAP_OUT]) plunger();
else if (part == "section") difference() {
  assembly();
  translate([bx(44.4), -1, -50]) cube([OUT_W, OUT_H + 2, 100]);
}
