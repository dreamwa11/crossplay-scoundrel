# Draws Scoundrel's "You died" picture for the PaperMono: a dungeon wall and
# floor behind (drawn at half strength later), a tombstone in front with a
# blank face for the game to write on, the hero's sword driven into the
# ground beside it, a skull and a guttered candle at its foot.
#
# Writes two SVGs: <out>_back.svg (wall and floor) and <out>_front.svg.
import math, random, sys
W, H = 480, 380
out = sys.argv[1]
rnd = random.Random(5)
FLOOR = 300  # where the wall meets the floor

def svg(body):
    return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}">'
            '<defs><pattern id="hatch" width="5" height="5" patternUnits="userSpaceOnUse" patternTransform="rotate(45)">'
            '<rect width="5" height="5" fill="#fff"/><rect width="1.6" height="5" fill="#000"/></pattern>'
            '<pattern id="dots" width="4" height="4" patternUnits="userSpaceOnUse"><rect width="4" height="4" fill="#fff"/>'
            '<rect x="1" y="1" width="1.4" height="1.4" fill="#000"/></pattern>'
            '<pattern id="dark" width="4" height="4" patternUnits="userSpaceOnUse"><rect width="4" height="4" fill="#000"/>'
            '<rect width="1.2" height="1.2" fill="#fff"/></pattern></defs>' + "".join(body) + '</svg>')

# ---- back: wall and floor ----------------------------------------------------
b = [f'<rect width="{W}" height="{H}" fill="#fff"/>']
course, y, row = 20, 0, 0
while y < FLOOR:
    x = -rnd.randint(0, 40) if row % 2 else 0
    while x < W:
        bw = rnd.randint(40, 70)
        y1 = min(y + course, FLOOR) - 1.5
        if y1 - (y + 1.5) > 4:
            r = rnd.random()
            fill = "url(#hatch)" if r < .12 else ("url(#dots)" if r < .26 else "#fff")
            b.append(f'<rect x="{x+1.5:.1f}" y="{y+1.5:.1f}" width="{bw-3:.1f}" height="{y1-y-1.5:.1f}" rx="2.5" fill="{fill}" stroke="#000" stroke-width="2"/>')
            if rnd.random() < .12:
                px, py = x + rnd.uniform(8, bw * .6), y + 1.5
                pts = [(px, py)]
                while py < y1 - 3:
                    px += rnd.uniform(-4, 6); py += rnd.uniform(3, 6); pts.append((px, min(py, y1)))
                b.append('<polyline points="' + " ".join(f"{a:.1f},{c:.1f}" for a, c in pts) + '" fill="none" stroke="#000" stroke-width="1.6"/>')
        x += bw
    y += course; row += 1
b.append(f'<rect x="0" y="{FLOOR-1}" width="{W}" height="3" fill="#000"/>')
cx = W / 2
for i in range(-10, 11):
    b.append(f'<line x1="{cx+i*24:.1f}" y1="{FLOOR}" x2="{cx+i*60:.1f}" y2="{H}" stroke="#000" stroke-width="1.6"/>')
for yy in (FLOOR + 14, FLOOR + 34, FLOOR + 60):
    b.append(f'<line x1="0" y1="{yy}" x2="{W}" y2="{yy}" stroke="#000" stroke-width="1.4"/>')
open(out + "_back.svg", "w").write(svg(b))

# ---- front: the grave ------------------------------------------------------------
f = []
# a mound of earth and rubble at the foot
f.append(f'<path d="M96,{H-22} C130,{FLOOR+2} 350,{FLOOR+2} 384,{H-22} Z" fill="url(#dots)" stroke="#000" stroke-width="3"/>')
for (rx, ry, rw, rh) in ((120, H - 36, 22, 12), (330, H - 34, 26, 13), (300, H - 26, 16, 9), (150, H - 26, 14, 8)):
    f.append(f'<rect x="{rx}" y="{ry}" width="{rw}" height="{rh}" rx="3" fill="#fff" stroke="#000" stroke-width="2"/>')
# the tombstone: an arched slab, a blank face, a chipped corner and a crack
sx, sw, top, base = cx - 92, 184, 46, FLOOR + 14
f.append(f'<path d="M{sx},{base} L{sx},{top+92} A92,92 0 0 1 {sx+sw},{top+92} L{sx+sw},{base} Z" fill="#fff" stroke="#000" stroke-width="4" stroke-linejoin="round"/>')
f.append(f'<path d="M{sx+12},{base} L{sx+12},{top+96} A80,80 0 0 1 {sx+sw-12},{top+96} L{sx+sw-12},{base}" fill="none" stroke="#000" stroke-width="2"/>')
f.append(f'<path d="M{sx+sw},{top+122} L{sx+sw-14},{top+128} L{sx+sw-6},{top+142} L{sx+sw},{top+144} Z" fill="#000"/>')
f.append(f'<polyline points="{sx+4},{top+150} {sx+12},{top+160} {sx+9},{top+172} {sx+16},{top+184}" fill="none" stroke="#000" stroke-width="2"/>')
f.append(f'<polyline points="{sx+sw-30},{base} {sx+sw-38},{base-16} {sx+sw-32},{base-28}" fill="none" stroke="#000" stroke-width="2"/>')
# moss along the bottom of the stone
f.append(f'<path d="M{sx},{base-24} C{sx+30},{base-34} {sx+50},{base-14} {sx+80},{base-22} L{sx+80},{base} L{sx},{base} Z" fill="url(#hatch)" stroke="#000" stroke-width="2"/>')
# the sword, driven in at an angle to the right of the stone
gx, gy = sx + sw + 66, FLOOR + 30
ang = math.radians(-12)
def rot(px, py):
    return (gx + px * math.cos(ang) - py * math.sin(ang), gy + px * math.sin(ang) + py * math.cos(ang))
def poly(pts, fill="#fff", sw_=3):
    return '<polygon points="' + " ".join(f"{a:.1f},{c:.1f}" for a, c in (rot(*p) for p in pts)) + f'" fill="{fill}" stroke="#000" stroke-width="{sw_}" stroke-linejoin="round"/>'
f.append(poly([(-7, 0), (-7, -150), (0, -160), (7, -150), (7, 0)]))            # blade
f.append(poly([(-1, -10), (-1, -146), (1, -146), (1, -10)], "#000", 1))       # fuller
f.append(poly([(-30, -160), (30, -160), (30, -170), (-30, -170)], "#000", 2))  # guard
f.append(poly([(-5, -170), (5, -170), (5, -206), (-5, -206)], "url(#hatch)", 2))  # grip
f.append(f'<circle cx="{rot(0,-214)[0]:.1f}" cy="{rot(0,-214)[1]:.1f}" r="9" fill="#000"/>')
# a skull at the left foot of the stone
kx, ky = sx - 40, FLOOR + 22
f.append(f'<path d="M{kx-24},{ky} C{kx-26},{ky-34} {kx+26},{ky-34} {kx+24},{ky} L{kx+16},{ky+4} L{kx+14},{ky+14} L{kx-14},{ky+14} L{kx-16},{ky+4} Z" fill="#fff" stroke="#000" stroke-width="3" stroke-linejoin="round"/>')
f.append(f'<ellipse cx="{kx-9}" cy="{ky-6}" rx="6.5" ry="7" fill="#000"/><ellipse cx="{kx+9}" cy="{ky-6}" rx="6.5" ry="7" fill="#000"/>')
f.append(f'<path d="M{kx},{ky+1} L{kx-3},{ky+6} L{kx+3},{ky+6} Z" fill="#000"/>')
for t in (-8, -3, 2, 7):
    f.append(f'<line x1="{kx+t}" y1="{ky+9}" x2="{kx+t}" y2="{ky+14}" stroke="#000" stroke-width="1.6"/>')
# a candle burned down to a stub, still lit, in front of the stone
cx2, cy2 = sx + sw - 40, base + 20
f.append(f'<rect x="{cx2-11}" y="{cy2-26}" width="22" height="28" rx="2" fill="#fff" stroke="#000" stroke-width="3"/>')
f.append(f'<path d="M{cx2-11},{cy2-24} C{cx2-14},{cy2-12} {cx2-6},{cy2-10} {cx2-8},{cy2-2}" fill="none" stroke="#000" stroke-width="2.5"/>')
f.append(f'<line x1="{cx2}" y1="{cy2-26}" x2="{cx2}" y2="{cy2-32}" stroke="#000" stroke-width="2"/>')
f.append(f'<path d="M{cx2},{cy2-62} C{cx2+6},{cy2-50} {cx2+13},{cy2-44} {cx2+10},{cy2-36} C{cx2+7},{cy2-30} {cx2-7},{cy2-30} {cx2-10},{cy2-36} C{cx2-13},{cy2-44} {cx2-4},{cy2-50} {cx2},{cy2-62} Z" fill="#000"/>')
f.append(f'<path d="M{cx2+1},{cy2-50} C{cx2+4},{cy2-44} {cx2+6},{cy2-41} {cx2+4},{cy2-37} C{cx2+2},{cy2-35} {cx2-3},{cy2-35} {cx2-4},{cy2-37} C{cx2-5},{cy2-41} {cx2-1},{cy2-44} {cx2+1},{cy2-50} Z" fill="#fff"/>')
open(out + "_front.svg", "w").write(svg(f).replace(f'<rect width="{W}" height="{H}" fill="#fff"/>', ''))
# The stone's blank face, for the code that writes on it.
print(f"face {int(sx+16)} {int(top+40)} {int(sw-32)} {int(base-24-(top+40))}")
