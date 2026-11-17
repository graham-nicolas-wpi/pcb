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
sv=shell.val()
print("shell valid",sv.isValid(),"solids",len(shell.solids().vals()),"vol",round(sv.Volume(),1))
# ---- board mock for renders/checks
board=box(X0-BW,X0,Y0-BH,Y0,ZB,ZB+BT)
for x,y in bpos: board=board.cut(cyl(x,y,3.2,ZB-1,ZB+BT+1))
for nm,o in [("plate∩shell",pv.intersect(sv)),("plate∩board",pv.intersect(board.val())),("shell∩board",sv.intersect(board.val()))]:
    print(nm,"overlap vol",round(o.Volume(),4))
import os
for nm,o in [("clock_front_plate",plate),("clock_shell",shell),("board_reference",board)]:
    cq.exporters.export(o,os.path.join(OUT,nm+".step"))
    cq.exporters.export(o,os.path.join(OUT,nm+".stl"),tolerance=0.02,angularTolerance=0.15)
asm=cq.Assembly(); asm.add(plate,name="front_plate",color=cq.Color(0.95,0.95,0.92)); asm.add(shell,name="shell",color=cq.Color(0.15,0.15,0.17)); asm.add(board,name="pcb_reference",color=cq.Color(0.1,0.45,0.2))
asm.save(os.path.join(OUT,"clock_assembly.step"))
