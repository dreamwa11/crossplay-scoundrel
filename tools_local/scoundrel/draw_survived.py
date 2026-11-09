# Draws Scoundrel's "You survived" picture for the PaperMono: the dungeon's
# last archway seen from inside (drawn at half strength later), and through
# it a sunlit forest clearing in full, with a rabbit in the grass.
#
# Writes <out>_back.svg (the dungeon) and <out>_front.svg (the outside).
import math, random, sys
W, H = 480, 380
out = sys.argv[1]
rnd = random.Random(11)
cx, R, SPRING = W / 2, 150, 176   # the opening: half-width, where the arch springs
FLOOR = 330

def svg(body, bg=True):
    head = (f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}">'
            '<defs><pattern id="hatch" width="5" height="5" patternUnits="userSpaceOnUse" patternTransform="rotate(45)">'
            '<rect width="5" height="5" fill="#fff"/><rect width="1.6" height="5" fill="#000"/></pattern>'
            '<pattern id="dots" width="4" height="4" patternUnits="userSpaceOnUse"><rect width="4" height="4" fill="#fff"/>'
            '<rect x="1" y="1" width="1.4" height="1.4" fill="#000"/></pattern>'
            f'<clipPath id="door"><path d="{opening()}"/></clipPath></defs>')
    return head + (f'<rect width="{W}" height="{H}" fill="#fff"/>' if bg else '') + "".join(body) + '</svg>'

def opening():
    return f'M{cx-R},{FLOOR} L{cx-R},{SPRING} A{R},{R} 0 0 1 {cx+R},{SPRING} L{cx+R},{FLOOR} Z'

# ---- back: the dungeon around the opening ----------------------------------------
b = []
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
        x += bw
    y += course; row += 1
# voussoirs round the opening
n, outer = 13, R + 22
for i in range(n):
    a0 = math.pi + i * math.pi / n; a1 = math.pi + (i + 1) * math.pi / n
    pts = [(cx + R * math.cos(a0), SPRING + R * math.sin(a0)), (cx + outer * math.cos(a0), SPRING + outer * math.sin(a0)),
           (cx + outer * math.cos(a1), SPRING + outer * math.sin(a1)), (cx + R * math.cos(a1), SPRING + R * math.sin(a1))]
    b.append('<polygon points="' + " ".join(f"{p:.1f},{q:.1f}" for p, q in pts) + '" fill="#fff" stroke="#000" stroke-width="2"/>')
for side in (-1, 1):
    yy, k = SPRING, 0
    while yy < FLOOR:
        x0, x1 = sorted((cx + side * R, cx + side * (outer + (6 if k % 2 else 0))))
        b.append(f'<rect x="{x0:.1f}" y="{yy:.1f}" width="{x1-x0:.1f}" height="{min(22, FLOOR-yy):.1f}" fill="#fff" stroke="#000" stroke-width="2"/>')
        yy += 22; k += 1
# the dungeon floor in front, flagstones running out of the door
b.append(f'<rect x="0" y="{FLOOR}" width="{W}" height="{H-FLOOR}" fill="#fff"/>')
b.append(f'<rect x="0" y="{FLOOR-1}" width="{W}" height="3" fill="#000"/>')
for i in range(-10, 11):
    b.append(f'<line x1="{cx+i*22:.1f}" y1="{FLOOR}" x2="{cx+i*52:.1f}" y2="{H}" stroke="#000" stroke-width="1.6"/>')
for yy in (FLOOR + 16, FLOOR + 36):
    b.append(f'<line x1="0" y1="{yy}" x2="{W}" y2="{yy}" stroke="#000" stroke-width="1.4"/>')
# the opening itself is left empty: the front layer fills it
b.append(f'<path d="{opening()}" fill="#fff" stroke="#000" stroke-width="3"/>')
open(out + "_back.svg", "w").write(svg(b))

# ---- front: the clearing outside ---------------------------------------------------------
f = [f'<g clip-path="url(#door)">', f'<rect x="0" y="0" width="{W}" height="{H}" fill="#fff"/>']
sx, sy = cx + 6, 150  # the sun, low in the gap between the trees
for k in range(22):
    a = -math.pi / 2 + (k - 10.5) * 0.15
    r0, r1 = 34, 190 + (k % 3) * 30
    f.append(f'<line x1="{sx+r0*math.cos(a):.1f}" y1="{sy+r0*math.sin(a):.1f}" x2="{sx+r1*math.cos(a):.1f}" y2="{sy+r1*math.sin(a):.1f}" stroke="#000" stroke-width="1.2" stroke-dasharray="{6+k%4},{9+k%3}"/>')
for k in range(14):  # rays falling down into the clearing
    a = math.pi / 2 + (k - 6.5) * 0.11
    f.append(f'<line x1="{sx+30*math.cos(a):.1f}" y1="{sy+30*math.sin(a):.1f}" x2="{sx+260*math.cos(a):.1f}" y2="{sy+260*math.sin(a):.1f}" stroke="#000" stroke-width="1" stroke-dasharray="3,11"/>')
f.append(f'<circle cx="{sx}" cy="{sy}" r="26" fill="#fff" stroke="#000" stroke-width="3"/>')
# far tree line: a row of rounded crowns
x = cx - R - 10
while x < cx + R + 10:
    r = rnd.uniform(16, 26)
    if abs(x - sx) > 46:
        f.append(f'<circle cx="{x:.1f}" cy="{214 - r*0.4:.1f}" r="{r:.1f}" fill="url(#dots)" stroke="#000" stroke-width="2"/>')
    x += r * 1.3
f.append(f'<rect x="0" y="212" width="{W}" height="{H}" fill="#fff"/>')
f.append(f'<line x1="0" y1="212" x2="{W}" y2="212" stroke="#000" stroke-width="2"/>')
# two big trees framing the view, their crowns a dotted canopy along the arch
def tree(tx, lean):
    f.append(f'<path d="M{tx-16},{FLOOR} C{tx-10},270 {tx-12+lean},190 {tx-8+lean},110 L{tx+8+lean},110 C{tx+12+lean},190 {tx+12},270 {tx+20},{FLOOR} Z" fill="url(#hatch)" stroke="#000" stroke-width="3"/>')
def canopy(blobs):
    for (x, y, r) in blobs:
        f.append(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="{r:.1f}" fill="#000"/>')
    for (x, y, r) in blobs:
        f.append(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="{r-3:.1f}" fill="url(#dots)"/>')
# the meadow: tufts of grass and a few flowers
for k in range(26):
    gx = cx - R + 8 + k * (2 * R - 16) / 25 + rnd.uniform(-4, 4)
    gy = rnd.uniform(240, FLOOR - 4)
    if cx + 40 < gx < cx + 96 and gy > 272: continue            # not on the rabbit
    f.append(f'<path d="M{gx-6:.1f},{gy:.1f} L{gx-2:.1f},{gy-9:.1f} L{gx:.1f},{gy:.1f} L{gx+3:.1f},{gy-11:.1f} L{gx+5:.1f},{gy:.1f}" fill="none" stroke="#000" stroke-width="1.6"/>')
for k in range(7):
    fx, fy = rnd.uniform(cx - R + 20, cx + R - 20), rnd.uniform(250, FLOOR - 10)
    f.append(f'<circle cx="{fx:.1f}" cy="{fy:.1f}" r="3.2" fill="#fff" stroke="#000" stroke-width="1.6"/><circle cx="{fx:.1f}" cy="{fy:.1f}" r="1" fill="#000"/>')
# a rabbit sitting in the light, facing the sun
rx, ry = cx + 70, 300
f.append(f'<ellipse cx="{rx}" cy="{ry-11}" rx="15" ry="12" fill="#000"/>')
f.append(f'<circle cx="{rx+12}" cy="{ry-24}" r="8" fill="#000"/>')
f.append(f'<ellipse cx="{rx+9}" cy="{ry-40}" rx="3.2" ry="11" transform="rotate(-12 {rx+9} {ry-40})" fill="#000"/>')
f.append(f'<ellipse cx="{rx+16}" cy="{ry-40}" rx="3.2" ry="11" transform="rotate(10 {rx+16} {ry-40})" fill="#000"/>')
f.append(f'<circle cx="{rx-15}" cy="{ry-12}" r="5" fill="#fff" stroke="#000" stroke-width="2"/>')
f.append(f'<circle cx="{rx+15}" cy="{ry-26}" r="1.5" fill="#fff"/>')
# the trees last, so nothing in the meadow shows through them
tree(cx - 108, -4)
tree(cx + 112, 6)
canopy([(cx - 150, 70, 40), (cx - 112, 92, 34), (cx - 140, 120, 30), (cx - 82, 112, 24), (cx - 168, 128, 30)])
canopy([(cx + 150, 74, 40), (cx + 118, 96, 32), (cx + 146, 122, 30), (cx + 88, 116, 22), (cx + 170, 132, 30)])
f.append('</g>')
f.append(f'<path d="{opening()}" fill="none" stroke="#000" stroke-width="3"/>')
open(out + "_front.svg", "w").write(svg(f, bg=False))
