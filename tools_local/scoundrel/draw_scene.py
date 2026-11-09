# Draws Scoundrel's room scene for the PaperMono as an SVG: a stone wall with
# varied, darkened, cracked and broken bricks, an arched doorway with
# voussoirs and a keystone, and two torches in iron sconces. Deterministic.
import random, math, sys
W, H = 480, 178
rnd = random.Random(int(sys.argv[2]) if len(sys.argv) > 2 else 7)
o = []
def add(s): o.append(s)
add(f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}">')
add('<defs>'
    '<pattern id="hatch" width="5" height="5" patternUnits="userSpaceOnUse" patternTransform="rotate(45)">'
    '<rect width="5" height="5" fill="#fff"/><rect width="1.6" height="5" fill="#000"/></pattern>'
    '<pattern id="dark" width="4" height="4" patternUnits="userSpaceOnUse">'
    '<rect width="4" height="4" fill="#000"/><rect x="0" y="0" width="1.2" height="1.2" fill="#fff"/></pattern>'
    '<pattern id="dots" width="4" height="4" patternUnits="userSpaceOnUse">'
    '<rect width="4" height="4" fill="#fff"/><rect x="1" y="1" width="1.3" height="1.3" fill="#000"/></pattern>'
    '</defs>')
add(f'<rect width="{W}" height="{H}" fill="#fff"/>')

floorY = 150
cx, doorW, doorTop = W / 2, 132, 40
r = doorW / 2
archR = r + 16  # outer edge of the voussoirs

def in_arch(x0, x1, y0, y1):
    # does a brick overlap the arch and its stones?
    if x1 < cx - archR - 2 or x0 > cx + archR + 2: return False
    springY = doorTop + r
    if y1 >= springY: return True
    for x in (x0, x1, (x0 + x1) / 2):
        dx = x - cx
        if abs(dx) <= archR + 2:
            yTop = springY - math.sqrt(max(0, (archR + 2) ** 2 - dx * dx))
            if y1 >= yTop: return True
    return False

# --- the wall ------------------------------------------------------------
course = 19
y = 0
row = 0
while y < floorY:
    x = -rnd.randint(0, 40) if row % 2 else 0
    while x < W:
        bw = rnd.randint(38, 66)
        x0, x1, y0, y1 = x + 1.5, x + bw - 1.5, y + 1.5, min(y + course, floorY) - 1.5
        if not in_arch(x0, x1, y0, y1) and y1 - y0 > 4:
            roll = rnd.random()
            fill = "url(#hatch)" if roll < 0.10 else ("url(#dots)" if roll < 0.22 else "#fff")
            add(f'<rect x="{x0:.1f}" y="{y0:.1f}" width="{x1-x0:.1f}" height="{y1-y0:.1f}" rx="2.5" fill="{fill}" stroke="#000" stroke-width="2"/>')
            roll2 = rnd.random()
            if roll2 < 0.12:  # a crack
                px, py = x0 + rnd.uniform(6, (x1 - x0) * 0.5), y0
                pts = [(px, py)]
                while py < y1 - 3:
                    px += rnd.uniform(-4, 6); py += rnd.uniform(3, 6)
                    pts.append((min(px, x1 - 2), min(py, y1)))
                add('<polyline points="' + " ".join(f"{a:.1f},{b:.1f}" for a, b in pts) + '" fill="none" stroke="#000" stroke-width="1.6"/>')
            elif roll2 < 0.19:  # a broken corner
                cw, chh = rnd.uniform(9, 16), rnd.uniform(6, y1 - y0 - 2)
                if rnd.random() < 0.5:
                    pts = [(x1, y0), (x1 - cw, y0), (x1 - cw * 0.4, y0 + chh * 0.6), (x1, y0 + chh)]
                else:
                    pts = [(x0, y1), (x0 + cw, y1), (x0 + cw * 0.5, y1 - chh * 0.5), (x0, y1 - chh)]
                add('<polygon points="' + " ".join(f"{a:.1f},{b:.1f}" for a, b in pts) + '" fill="#000"/>')
            elif roll2 < 0.22:  # a few chips along the top edge
                for _ in range(2):
                    px = rnd.uniform(x0 + 4, x1 - 6)
                    add(f'<polygon points="{px:.1f},{y0:.1f} {px+5:.1f},{y0:.1f} {px+2:.1f},{y0+4:.1f}" fill="#000"/>')
        x += bw
    y += course
    row += 1

# --- the floor -------------------------------------------------------------
add(f'<rect x="0" y="{floorY}" width="{W}" height="{H-floorY}" fill="#fff"/>')
add(f'<rect x="0" y="{floorY-1}" width="{W}" height="3" fill="#000"/>')
# flagstones in perspective: lines converging on the doorway
for i in range(-9, 10):
    xb = cx + i * 46
    xt = cx + i * 20
    add(f'<line x1="{xt:.1f}" y1="{floorY}" x2="{xb:.1f}" y2="{H}" stroke="#000" stroke-width="1.6"/>')
for k, yy in enumerate([floorY + 9, floorY + 20]):
    add(f'<line x1="0" y1="{yy}" x2="{W}" y2="{yy}" stroke="#000" stroke-width="1.4"/>')

# --- the doorway ---------------------------------------------------------
springY = doorTop + r
# voussoirs: wedge stones around the arch, a keystone at the top
n = 11
for i in range(n):
    a0 = math.pi + i * math.pi / n
    a1 = math.pi + (i + 1) * math.pi / n
    pts = [(cx + r * math.cos(a0), springY + r * math.sin(a0)), (cx + archR * math.cos(a0), springY + archR * math.sin(a0)),
           (cx + archR * math.cos(a1), springY + archR * math.sin(a1)), (cx + r * math.cos(a1), springY + r * math.sin(a1))]
    key = i == n // 2
    if key:  # the keystone stands proud
        am = (a0 + a1) / 2
        pts[1] = (cx + (archR + 6) * math.cos(a0), springY + (archR + 6) * math.sin(a0))
        pts[2] = (cx + (archR + 6) * math.cos(a1), springY + (archR + 6) * math.sin(a1))
    fill = "url(#dots)" if (i % 3 == 1 and not key) else "#fff"
    add('<polygon points="' + " ".join(f"{a:.1f},{b:.1f}" for a, b in pts) + f'" fill="{fill}" stroke="#000" stroke-width="2" stroke-linejoin="round"/>')
# jamb stones down both sides
for side in (-1, 1):
    xIn = cx + side * r
    xOut = cx + side * archR
    yy = springY
    k = 0
    while yy < floorY:
        hh = 18
        x0, x1 = sorted((xIn, xOut + side * (5 if k % 2 else 0)))
        add(f'<rect x="{x0:.1f}" y="{yy:.1f}" width="{x1-x0:.1f}" height="{min(hh, floorY-yy):.1f}" fill="#fff" stroke="#000" stroke-width="2"/>')
        yy += hh; k += 1
# the dark opening
add(f'<path d="M{cx-r:.1f},{floorY} L{cx-r:.1f},{springY:.1f} A{r:.1f},{r:.1f} 0 0 1 {cx+r:.1f},{springY:.1f} L{cx+r:.1f},{floorY} Z" fill="url(#dark)" stroke="#000" stroke-width="3"/>')
# a faint glimpse of steps into the dark
for i, yy in enumerate([floorY - 22, floorY - 12]):
    w = r * (0.55 + i * 0.2)
    add(f'<line x1="{cx-w:.1f}" y1="{yy}" x2="{cx+w:.1f}" y2="{yy}" stroke="#fff" stroke-width="1.4"/>')

# --- torches -----------------------------------------------------------------
def torch(tx, ty):
    # iron sconce: a plate on the wall, an arm, a cup
    add(f'<rect x="{tx-7}" y="{ty+18}" width="14" height="24" rx="3" fill="#000"/>')
    add(f'<path d="M{tx},{ty+30} L{tx},{ty+14}" stroke="#000" stroke-width="4"/>')
    add(f'<path d="M{tx-11},{ty+6} L{tx+11},{ty+6} L{tx+6},{ty+16} L{tx-6},{ty+16} Z" fill="#000"/>')
    # the flame: an outer tongue, a white inner one
    add(f'<path d="M{tx},{ty-26} C{tx+5},{ty-14} {tx+15},{ty-8} {tx+11},{ty+2} C{tx+8},{ty+8} {tx-8},{ty+8} {tx-11},{ty+2} '
        f'C{tx-14},{ty-7} {tx-3},{ty-12} {tx},{ty-26} Z" fill="#000"/>')
    add(f'<path d="M{tx+1},{ty-14} C{tx+4},{ty-7} {tx+7},{ty-4} {tx+5},{ty+1} C{tx+3},{ty+4} {tx-3},{ty+4} {tx-5},{ty+1} '
        f'C{tx-6},{ty-4} {tx-1},{ty-7} {tx+1},{ty-14} Z" fill="#fff"/>')
    # light: a white halo cleared out of the wall, with short rays
    for k in range(10):
        ang = -math.pi / 2 + (k - 4.5) * 0.32
        r0, r1 = 31, 39
        add(f'<line x1="{tx+r0*math.cos(ang):.1f}" y1="{ty-6+r0*math.sin(ang):.1f}" x2="{tx+r1*math.cos(ang):.1f}" y2="{ty-6+r1*math.sin(ang):.1f}" stroke="#000" stroke-width="1.6" stroke-linecap="round"/>')
for tx in (88, W - 88):
    add(f'<circle cx="{tx}" cy="{56}" r="27" fill="#fff"/>')
    torch(tx, 62)

add('</svg>')
open(sys.argv[1], "w").write("\n".join(o))
