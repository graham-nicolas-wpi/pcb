"""Clock face plate + open-back shell. Coordinates match Nicolas's Fusion STEP
(face at z=0, viewer looks from -z, board top = +Y, board left/USB = +X)."""
import cadquery as cq, sys
SRC=sys.argv[1]; OUT=sys.argv[2]
# ---- board placement in STEP coords (fitted from LED positions vs segment cells)
X0,Y0=9.32,76.75                 # board top-left corner; X = X0-bx, Y = Y0-by
BW,BH,BT=102.0,40.0,1.6
ZB=5.0                           # board front face rests on light-guide wall tops
HOLES=[(86.0,4.0),(48.0,31.4),(84.3,35.7),(65.7,4.8),(33.6,2.7)]   # 3.2 mm NPTH
USB_BY=15.31; C1=(4.0,7.5)
# ---- parameters
T=1.2                            # face thickness (same as original)
ML=6.0; MB=12.0                  # margin left of board / below board
XC=-57.28; YC=57.83              # digit centre -> plate is centred on the digits
WALL=3.0; CLR=0.25; RIB=1.6; RIB_H=3.0; R=4.0
INS_D=4.0                        # heat-set insert hole (M3, OD 4.2-4.6)
B_OD=7.5; B_FLOOR=1.0            # board bosses
S_OD=8.0; S_TOP=8.0; S_DEPTH=6.0 # shell bosses
EAR_T=3.0; EAR_W=10.0; EAR_GAP=0.2
DEPTH=30.0                       # shell depth behind the face
x1=X0+ML; x0=2*XC-x1; y0=Y0-BH-MB; y1=2*YC-y0
print("plate",round(x1-x0,2),"x",round(y1-y0,2), "X",x0,x1,"Y",y0,y1)
def rrect(xa,xb,ya,yb,za,zb,r):
    w=cq.Workplane("XY").workplane(offset=za).center((xa+xb)/2,(ya+yb)/2).rect(xb-xa,yb-ya).extrude(zb-za)
    return w.edges("|Z").fillet(r) if r>0 else w
def box(xa,xb,ya,yb,za,zb): return rrect(xa,xb,ya,yb,za,zb,0)
def cyl(x,y,d,za,zb): return cq.Workplane("XY").workplane(offset=za).center(x,y).circle(d/2).extrude(zb-za)
# ---- digit core, cropped out of the original untouched
orig=cq.importers.importStep(SRC).val()
DX0,DX1,DY0,DY1=-93.2,-21.4,42.4,73.2
core=orig.intersect(box(DX0,DX1,DY0,DY1,0,ZB).val())
# filled silhouettes of the light-guide blocks (to keep new material out of the cells)
tops=[f for f in core.faces().vals() if hasattr(f,'normalAt')] if False else [f for f in core.Faces() if f.geomType()=="PLANE" and abs(f.Center().z-ZB)<1e-3]
filled=None
for f in tops:
    s=cq.Solid.extrudeLinear(cq.Face.makeFromWires(f.outerWire()),cq.Vector(0,0,-(ZB-T)))
    filled=s if filled is None else filled.fuse(s)
filled=cq.Workplane(obj=filled)
# ---- plate base
plate=rrect(x0,x1,y0,y1,0,T,R).cut(box(DX0,DX1,DY0,DY1,0,T))
# perimeter locating rib
ro=WALL+CLR
rib=rrect(x0+ro,x1-ro,y0+ro,y1-ro,T,T+RIB_H,R-ro+0.01 if R>ro else 0.5).cut(rrect(x0+ro+RIB,x1-ro-RIB,y0+ro+RIB,y1-ro-RIB,T,T+RIB_H,0.5))
plate=plate.union(rib)
# board bosses
bpos=[(X0-bx,Y0-by) for bx,by in HOLES]
for x,y in bpos: plate=plate.union(cyl(x,y,B_OD,T,ZB).cut(filled))
# shell bosses (top and bottom strips)
sy_b=(Y0-BH)-0.6-S_OD/2; sy_t=Y0+0.6+S_OD/2
spos=[(XC+57.28,sy_t),(XC-57.28,sy_t),(XC+57.28,sy_b),(XC-57.28,sy_b)]
for x,y in spos:
    plate=plate.union(cyl(x,y,S_OD,T,S_TOP))
    # web tying the boss to the rib
    yr=(y1-ro-RIB/2) if y>YC else (y0+ro+RIB/2)
    plate=plate.union(box(x-1.2,x+1.2,min(y,yr),max(y,yr),T,T+RIB_H+1.5))
for x,y in bpos: plate=plate.cut(cyl(x,y,INS_D,B_FLOOR,ZB))
for x,y in spos: plate=plate.cut(cyl(x,y,INS_D,S_TOP-S_DEPTH,S_TOP))
# USB plug notch + C1 capacitor hole
uy=Y0-USB_BY
SKIN=0.4  # face stays solid; hidden reliefs on the inside only
plate=plate.cut(box(X0-0.3,x1+1,uy-6.5,uy+6.5,SKIN,T+RIB_H+1))
plate=plate.cut(cyl(X0-C1[0],Y0-C1[1],6.2,SKIN,T+0.01))
plate=plate.union(cq.Workplane(obj=core)).clean()
pv=plate.val()
print("plate valid",pv.isValid(),"solids",len(plate.solids().vals()),"vol",round(pv.Volume(),1))
# ---- shell (open back ring)
zf=T; zb=T+DEPTH
shell=rrect(x0,x1,y0,y1,zf,zb,R).cut(rrect(x0+WALL,x1-WALL,y0+WALL,y1-WALL,zf-1,zb+1,max(R-WALL,0.5)))
ez0=S_TOP+EAR_GAP; ez1=ez0+EAR_T
for x,y in spos:
    top=y>YC
    yw=(y1-WALL) if top else (y0+WALL)          # inner wall face
    yi=y-(S_OD/2+0.5) if top else y+(S_OD/2+0.5) # inner end of ear
    L=abs(yw-yi)
    # ear + 45 deg gusset behind it (prints back-down with no support)
    pts=[(yw,ez0),(yi,ez0),(yi,ez1),(yw,ez1+L)]
    g=cq.Workplane("YZ").workplane(offset=x-EAR_W/2).polyline(pts).close().extrude(EAR_W)
    g=g.union(box(x-EAR_W/2,x+EAR_W/2,min(yw,yw+(0.5 if top else -0.5)),max(yw,yw+(0.5 if top else -0.5)),ez0,ez1+L))
    g=g.cut(cyl(x,y,3.4,ez0-1,ez1+1)).cut(cyl(x,y,6.6,ez1,zb+1))
    shell=shell.union(g)
# USB notch in the left wall, open to the front edge
shell=shell.cut(box(x1-WALL-1,x1+1,uy-6.5,uy+6.5,zf-1,11.0)).clean()
# ---- rotary encoder (KY-040) through the TOP wall, shaft pointing up (+Y)
ENC_X=-85.0                      # above the right-hand digit as seen from the front
ENC_Z=zf+DEPTH/2                 # middle of the shell depth
ENC_HOLE=7.5                     # M7 bushing
ENC_POCKET=13.0; ENC_WALL=1.5    # square pocket for the 12x12 body, leaves 1.5 mm wall for the nut
yi_top=y1-WALL                   # inner face of the top wall
shell=shell.cut(cq.Workplane("XZ",origin=(ENC_X,y1+1,ENC_Z)).circle(ENC_HOLE/2).extrude(WALL+2))
shell=shell.cut(box(ENC_X-ENC_POCKET/2,ENC_X+ENC_POCKET/2,yi_top-0.5,y1-ENC_WALL,ENC_Z-ENC_POCKET/2,ENC_Z+ENC_POCKET/2))
sv=shell.val()
print("shell valid",sv.isValid(),"solids",len(shell.solids().vals()),"vol",round(sv.Volume(),1))
# ---- board mock for renders/checks
board=box(X0-BW,X0,Y0-BH,Y0,ZB,ZB+BT)
for x,y in bpos: board=board.cut(cyl(x,y,3.2,ZB-1,ZB+BT+1))
for nm,o in [("plate∩shell",pv.intersect(sv)),("plate∩board",pv.intersect(board.val())),("shell∩board",sv.intersect(board.val()))]:
    print(nm,"overlap vol",round(o.Volume(),4))
# ---- encoder mock (12x12x6.5 body, 1.6 PCB 18.6 x 26.6, bushing 7 mm) for checks/renders
ey=y1-ENC_WALL                    # encoder body top sits against the thinned wall
enc=box(ENC_X-6,ENC_X+6,ey-6.5,ey,ENC_Z-6,ENC_Z+6)
enc=enc.union(box(ENC_X-13.3,ENC_X+5.3,ey-8.1,ey-6.5,ENC_Z-9.3,ENC_Z+9.3))
enc=enc.union(cq.Workplane("XZ",origin=(ENC_X,ey,ENC_Z)).circle(3.5).extrude(-5.0))
enc=enc.union(cq.Workplane("XZ",origin=(ENC_X,ey+5.0,ENC_Z)).circle(3.0).extrude(-11.0))   # shaft tip ~16 mm above the body (photo 0068)
print("shell∩encoder overlap vol",round(sv.intersect(enc.val()).Volume(),4),
      " encoder∩board",round(enc.val().intersect(board.val()).Volume(),4),
      " encoder∩plate",round(enc.val().intersect(pv).Volume(),4))
# ---- ESP32 DevKit bay (right of the board as seen from the front) + clip-on cap
# The ESP32-WROOM-32E DevKitC (38-pin, rows 25.4 mm apart) hangs inside the shell from a cap
# that presses into the back of the bay. Its header pins go through two slots in the cap so the
# jumper wires plug in from the open back; the jumpers hold the DevKit against the cap ledges.
DK_W,DK_L,DK_T=28.0,52.0,1.6      # DevKit PCB (antenna sticks ~6 mm past one end)
DK_ANT=6.0
DK_ROW=25.4; DK_HDR=2.5           # pin row spacing, header plastic height
DK_XC=-110.0                      # DevKit centre X; USB end down (-Y)
DK_Y0=y0+WALL+0.75                # USB edge of the PCB
CAP_T=3.0; CAP_X1=-88.0           # cap thickness, inner (board-side) edge of the cap
LIP_T=1.6; LIP_D=6.0; LIP_C=0.2   # press-fit lip inside the walls
SLOT_W=1.2; RELIEF_W=3.4; LEDGE=1.0
USB_W=13.0; USB_Z0=18.0           # micro-USB cable slot in the bottom wall, open to the back edge
dk_y1=DK_Y0+DK_L
# shell: USB cable slot in the bottom wall
shell=shell.cut(box(DK_XC-USB_W/2,DK_XC+USB_W/2,y0-1,y0+WALL+1,USB_Z0,zb+1)).clean()
sv=shell.val()
# cap
cap=rrect(x0,CAP_X1+4,y0,y1,zb,zb+CAP_T,R).cut(box(CAP_X1,CAP_X1+5,y0-1,y1+1,zb-1,zb+CAP_T+1))
xi0=x0+WALL+LIP_C; yi0=y0+WALL+LIP_C; yi1=y1-WALL-LIP_C
lip=box(xi0,xi0+LIP_T,yi0+1.2,yi1-1.2,zb-LIP_D,zb)                                   # right wall
lip=lip.union(box(xi0+1.2,DK_XC-DK_W/2-1.0,yi0,yi0+LIP_T,zb-LIP_D,zb))            # bottom wall, both sides
lip=lip.union(box(DK_XC+DK_W/2+1.0,CAP_X1,yi0,yi0+LIP_T,zb-LIP_D,zb))         #   of the DevKit
lip=lip.union(box(xi0+1.2,DK_XC-DK_W/2-1.0,yi1-LIP_T,yi1,zb-LIP_D,zb))            # top wall (clear of antenna)
cap=cap.union(lip)
for sx in (DK_XC-DK_ROW/2,DK_XC+DK_ROW/2):
    cap=cap.cut(box(sx-SLOT_W/2,sx+SLOT_W/2,DK_Y0+1.0,dk_y1-1.0,zb-1,zb+CAP_T+1))          # pins
    cap=cap.cut(box(sx-RELIEF_W/2,sx+RELIEF_W/2,DK_Y0+1.0,dk_y1-1.0,zb+LEDGE,zb+CAP_T+1))   # jumper housings
capv=cap.val()
print("cap valid",capv.isValid(),"solids",len(cap.solids().vals()),"vol",round(capv.Volume(),1),
      "size",round(CAP_X1-x0,1),"x",round(y1-y0,1))
# DevKit mock: header plastic against the cap, PCB, module + antenna toward the face, USB plug
zh=zb-DK_HDR; zp=zh-DK_T
dk=box(DK_XC-DK_W/2,DK_XC+DK_W/2,DK_Y0,dk_y1,zp,zh)
for sx in (DK_XC-DK_ROW/2,DK_XC+DK_ROW/2):
    dk=dk.union(box(sx-1.25,sx+1.25,DK_Y0+1.5,DK_Y0+1.5+19*2.54,zh,zb))
dk=dk.union(box(DK_XC-9,DK_XC+9,dk_y1-19.5,dk_y1+DK_ANT,zp-3.2,zp))               # module
dk=dk.union(box(DK_XC-4,DK_XC+4,DK_Y0-0.5,DK_Y0+5,zp-3.0,zp))                     # micro-USB socket
plug=box(DK_XC-5.5,DK_XC+5.5,y0-20,DK_Y0-0.5,zp-1.5-4,zp-1.5+4)                   # plug overmold
for nm,a,b in [("devkit∩shell",dk,sv),("devkit∩cap",dk,capv),("devkit∩encoder",dk,enc.val()),
               ("devkit∩board",dk,board.val()),("devkit∩plate",dk,pv),("cap∩shell",cap,sv),
               ("plug∩shell",plug,sv),("cap∩encoder",cap,enc.val())]:
    av=a.val() if hasattr(a,"val") else a
    o=av.intersect(b); print(nm,"overlap vol",round(o.Volume(),4),[round(v,2) for v in (o.BoundingBox().xmin,o.BoundingBox().xmax,o.BoundingBox().ymin,o.BoundingBox().ymax,o.BoundingBox().zmin,o.BoundingBox().zmax)] if o.Volume()>1e-4 else "")
# ---- knob: 20 mm, 16 mm tall, 12 subtle waves with a slight twist, 6 mm D bore
import math
K_R=10.0; K_AMP=0.35; K_N=12; K_H=16.0; K_TWIST=12.0
pts=[((K_R+K_AMP*math.cos(K_N*t))*math.cos(t),(K_R+K_AMP*math.cos(K_N*t))*math.sin(t))
     for t in [2*math.pi*i/360 for i in range(360)]]
knob=cq.Workplane("XY").spline(pts,periodic=True).close().twistExtrude(K_H,K_TWIST)
knob=knob.faces(">Z").edges().chamfer(0.8)
knob=knob.cut(cyl(0,0,12.5,-1,4.0))                     # clears the M7 nut and the bushing end
dbore=cq.Workplane("XY").workplane(offset=-1).circle(6.1/2).extrude(14.8)  # bore up to z=13.8
dbore=dbore.cut(box(-5,5,-5,-6.1/2+(6.1-4.5),-2,15))    # D flat: 4.5 mm flat-to-round (measured 4.37)
knob=knob.cut(dbore)
knob=knob.cut(box(-0.6,0.6,2.5,K_R-1.2,K_H-0.6,K_H+1))   # pointer groove on the top face
kv=knob.val(); print("knob valid",kv.isValid(),"vol",round(kv.Volume(),1))
# knob placed on the encoder for the assembly: axis along +Y, 1 mm above the shell
knob_pl=knob.rotate((0,0,0),(1,0,0),-90).translate((ENC_X,y1+1.0,ENC_Z))
import os
for nm,o in [("clock_front_plate",plate),("clock_shell",shell),("clock_esp32_cap",cap),("esp32_devkit_reference",dk),("board_reference",board),("clock_knob",knob),("encoder_reference",enc)]:
    cq.exporters.export(o,os.path.join(OUT,nm+".step"))
    cq.exporters.export(o,os.path.join(OUT,nm+".stl"),tolerance=0.02,angularTolerance=0.15)
asm=cq.Assembly(); asm.add(plate,name="front_plate",color=cq.Color(0.95,0.95,0.92)); asm.add(shell,name="shell",color=cq.Color(0.15,0.15,0.17)); asm.add(board,name="pcb_reference",color=cq.Color(0.1,0.45,0.2)); asm.add(enc,name="encoder_reference",color=cq.Color(0.6,0.6,0.62)); asm.add(knob_pl,name="knob",color=cq.Color(0.95,0.95,0.92)); asm.add(cap,name="esp32_cap",color=cq.Color(0.15,0.15,0.17)); asm.add(dk,name="esp32_devkit_reference",color=cq.Color(0.1,0.1,0.35))
asm.save(os.path.join(OUT,"clock_assembly.step"))
