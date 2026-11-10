# Draws Scoundrel's room scenes for the PaperMono, one SVG per kind of room,
# after the phone version's paintings. Each is 480x178 and is knocked out to
# half strength later, so the cards stay in front.
#
#   hall     the old one: a doorway in a brick wall between two torches
#   window   a corner of a room: a barred window throwing light on the floor,
#            a plank door in the side wall, a lantern
#   cellar   low beams overhead, candles on the wall, barrels and a ladder
#   crypt    arches receding down a pillared vault to a small lit window
#
# usage: draw_rooms.py <kind> <out.svg>
import math, random, sys

W, H = 480, 178
kind, out = sys.argv[1], sys.argv[2]
rnd = random.Random({"hall": 7, "window": 3, "cellar": 5, "crypt": 9}.get(kind, 1))
o = []
add = o.append

DEFS = ('<defs>'
        '<pattern id="hatch" width="5" height="5" patternUnits="userSpaceOnUse" patternTransform="rotate(45)">'
        '<rect width="5" height="5" fill="#fff"/><rect width="1.6" height="5" fill="#000"/></pattern>'
        '<pattern id="wood" width="6" height="6" patternUnits="userSpaceOnUse">'
        '<rect width="6" height="6" fill="#fff"/><rect width="6" height="1.4" fill="#000"/></pattern>'
        '<pattern id="dots" width="4" height="4" patternUnits="userSpaceOnUse"><rect width="4" height="4" fill="#fff"/>'
        '<rect x="1" y="1" width="1.3" height="1.3" fill="#000"/></pattern>'
        '<pattern id="dark" width="4" height="4" patternUnits="userSpaceOnUse"><rect width="4" height="4" fill="#000"/>'
        '<rect width="1.2" height="1.2" fill="#fff"/></pattern></defs>')


def poly(pts, fill="#fff", sw=2):
    add('<polygon points="' + " ".join(f"{x:.1f},{y:.1f}" for x, y in pts) +
        f'" fill="{fill}" stroke="#000" stroke-width="{sw}" stroke-linejoin="round"/>')


def line(x1, y1, x2, y2, sw=1.6):
    add(f'<line x1="{x1:.1f}" y1="{y1:.1f}" x2="{x2:.1f}" y2="{y2:.1f}" stroke="#000" stroke-width="{sw}"/>')


def bricks(x0, y0, x1, y1, course=16, wmin=30, wmax=52, marks=True):
    """A flat wall of coursed bricks, some shaded, some cracked."""
    y, row = y0, 0
    while y < y1 - 3:
        x = x0 - (rnd.randint(0, 24) if row % 2 else 0)
        while x < x1:
            bw = rnd.randint(wmin, wmax)
            a, b = max(x, x0) + 1.2, min(x + bw, x1) - 1.2
            yb = min(y + course, y1) - 1.2
            if b - a > 5 and yb - y > 4:
                r = rnd.random()
                fill = "url(#hatch)" if marks and r < .1 else ("url(#dots)" if marks and r < .22 else "#fff")
                add(f'<rect x="{a:.1f}" y="{y+1.2:.1f}" width="{b-a:.1f}" height="{yb-y-1.2:.1f}" rx="2" fill="{fill}" stroke="#000" stroke-width="1.8"/>')
                if marks and rnd.random() < .1:
                    px = a + rnd.uniform(4, (b - a) * .6)
                    add(f'<polyline points="{px:.1f},{y+1.5:.1f} {px+3:.1f},{(y+yb)/2:.1f} {px:.1f},{yb:.1f}" fill="none" stroke="#000" stroke-width="1.4"/>')
            x += bw
        y += course
        row += 1


def side_wall(xb, yt, yb, xo, yto, ybo, course=16):
    """A side wall in perspective: from the back wall's edge (xb) out to the
    frame's edge (xo). Courses converge on the back wall."""
    poly([(xb, yt), (xo, yto), (xo, ybo), (xb, yb)], "#fff", 2)
    n = int((yb - yt) / course)
    for i in range(1, n + 1):
        t = i / ((yb - yt) / course)
        line(xb, yt + (yb - yt) * t, xo, yto + (ybo - yto) * t, 1.4)
        # a few joints per course, staggered
        for k in range(3):
            u = (k + (0.5 if i % 2 else 0) + .3) / 3.2
            x = xb + (xo - xb) * u
            ya = yt + (yb - yt) * (t - 1 / ((yb - yt) / course))
            ya2 = yto + (ybo - yto) * (t - 1 / ((yb - yt) / course))
            yc = yt + (yb - yt) * t
            yc2 = yto + (ybo - yto) * t
            line(x, ya + (ya2 - ya) * u, x, yc + (yc2 - yc) * u, 1.2)


def floor(y0, vx, vy, n=12, rows=(9, 21, 38)):
    """Flagstones in perspective, converging on (vx, vy)."""
    add(f'<rect x="0" y="{y0}" width="{W}" height="{H-y0}" fill="#fff"/>')
    line(0, y0, W, y0, 2.4)
    for i in range(-n, n + 1):
        xb = vx + i * 46
        # where the line toward the vanishing point crosses y0
        xt = vx + (xb - vx) * (y0 - vy) / (H - vy)
        line(xt, y0, xb, H)
    for r in rows:
        if y0 + r < H:
            line(0, y0 + r, W, y0 + r, 1.3)


def torch(tx, ty, scale=1.0):
    s = scale
    add(f'<circle cx="{tx}" cy="{ty-6*s}" r="{25*s}" fill="#fff"/>')
    add(f'<rect x="{tx-6*s}" y="{ty+16*s}" width="{12*s}" height="{20*s}" rx="3" fill="#000"/>')
    add(f'<path d="M{tx},{ty+26*s} L{tx},{ty+13*s}" stroke="#000" stroke-width="{4*s}"/>')
    add(f'<path d="M{tx-10*s},{ty+5*s} L{tx+10*s},{ty+5*s} L{tx+5*s},{ty+14*s} L{tx-5*s},{ty+14*s} Z" fill="#000"/>')
    add(f'<path d="M{tx},{ty-24*s} C{tx+5*s},{ty-13*s} {tx+14*s},{ty-7*s} {tx+10*s},{ty+2*s} C{tx+7*s},{ty+7*s} {tx-7*s},{ty+7*s} {tx-10*s},{ty+2*s} '
        f'C{tx-13*s},{ty-6*s} {tx-3*s},{ty-11*s} {tx},{ty-24*s} Z" fill="#000"/>')
    add(f'<path d="M{tx+1*s},{ty-12*s} C{tx+4*s},{ty-6*s} {tx+6*s},{ty-3*s} {tx+4*s},{ty+1*s} C{tx+2*s},{ty+4*s} {tx-3*s},{ty+4*s} {tx-4*s},{ty+1*s} '
        f'C{tx-5*s},{ty-3*s} {tx-1*s},{ty-6*s} {tx+1*s},{ty-12*s} Z" fill="#fff"/>')


def candle(cx, cy):
    add(f'<rect x="{cx-4}" y="{cy-12}" width="8" height="14" fill="#fff" stroke="#000" stroke-width="2"/>')
    add(f'<path d="M{cx},{cy-28} C{cx+4},{cy-21} {cx+6},{cy-18} {cx+4},{cy-15} C{cx+2},{cy-13} {cx-2},{cy-13} {cx-4},{cy-15} C{cx-6},{cy-18} {cx-3},{cy-21} {cx},{cy-28} Z" fill="#000"/>')


def arch_window(cx, top, w, h, bars=True):
    """An arched window, lit: white glass, black bars, a stone frame."""
    r = w / 2
    sp = top + r
    stone = 9
    add(f'<path d="M{cx-r-stone},{top+h+4} L{cx-r-stone},{sp} A{r+stone},{r+stone} 0 0 1 {cx+r+stone},{sp} L{cx+r+stone},{top+h+4} Z" fill="url(#dots)" stroke="#000" stroke-width="2"/>')
    add(f'<path d="M{cx-r},{top+h} L{cx-r},{sp} A{r},{r} 0 0 1 {cx+r},{sp} L{cx+r},{top+h} Z" fill="#fff" stroke="#000" stroke-width="2.5"/>')
    if bars:
        for k in range(1, 4):
            x = cx - r + w * k / 4
            dy = math.sqrt(max(0, r * r - (x - cx) ** 2))
            line(x, sp - dy, x, top + h, 2.2)
        for k in range(1, 4):
            y = sp + (top + h - sp) * k / 4
            line(cx - r, y, cx + r, y, 2.2)
    add(f'<rect x="{cx-r-stone-3}" y="{top+h}" width="{w+2*stone+6}" height="6" fill="#fff" stroke="#000" stroke-width="2"/>')


add(f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}">' + DEFS)
add(f'<rect width="{W}" height="{H}" fill="#fff"/>')

if kind == "window":
    # A corner: the back wall runs from the left side wall across to the right edge.
    BX, FY = 120, 132
    bricks(BX, 0, W, FY)
    side_wall(BX, 0, FY, 0, -40, H)
    floor(FY, 300, 40)
    # the window, right of centre, and its light on the floor
    cx, top, w, h = 330, 22, 74, 86
    arch_window(cx, top, w, h)
    lp = [(cx - 44, FY + 6), (cx + 44, FY + 6), (cx + 86, H - 4), (cx - 10, H - 4)]
    poly(lp, "#fff", 1.4)
    for k in range(1, 4):
        u = k / 4
        line(lp[0][0] + (lp[1][0] - lp[0][0]) * u, lp[0][1], lp[3][0] + (lp[2][0] - lp[3][0]) * u, lp[3][1], 3)
    line(lp[0][0] + 6, (lp[0][1] + lp[3][1]) / 2, lp[1][0] + 26, (lp[0][1] + lp[3][1]) / 2, 3)
    # a plank door with an iron ring in the side wall
    d = [(44, 72), (92, 50), (92, 132), (44, 158)]
    add(f'<path d="M44,158 L44,80 Q66,48 92,52 L92,132 Z" fill="url(#wood)" stroke="#000" stroke-width="2.5"/>')
    for k in (1, 2):
        x = 44 + 48 * k / 3
        line(x, 72 - 10 * k, x, 158 - 26 * k / 3 * 1.0 - (k * 0), 1.4)
    add('<circle cx="80" cy="108" r="4" fill="#fff" stroke="#000" stroke-width="2"/>')
    # a lantern on a bracket
    add('<path d="M196,40 L196,30 L216,30" fill="none" stroke="#000" stroke-width="3"/>')
    add('<rect x="208" y="34" width="16" height="20" rx="2" fill="#fff" stroke="#000" stroke-width="2.5"/>')
    add('<path d="M206,34 L216,26 L226,34 Z" fill="#000"/>')
    candle(216, 52)

elif kind == "cellar":
    BX0, BX1, FY = 70, 410, 130
    bricks(BX0, 24, BX1, FY)
    side_wall(BX0, 24, FY, 0, 0, H)
    side_wall(BX1, 24, FY, W, 0, H)
    floor(FY, 240, 60)
    # beams overhead: one across the front, joists running back
    poly([(0, 0), (W, 0), (BX1, 24), (BX0, 24)], "#fff", 2)
    for i in range(-3, 4):
        xb = 240 + i * 52
        xf = 240 + i * 92
        poly([(xf - 9, 0), (xf + 9, 0), (xb + 5, 24), (xb - 5, 24)], "url(#wood)", 1.6)
    add(f'<rect x="0" y="22" width="{W}" height="12" fill="url(#wood)" stroke="#000" stroke-width="2"/>')
    for x in (30, W - 30):
        poly([(x - 6, 34), (x + 6, 34), (x + 6, H), (x - 6, H)], "url(#wood)", 2)
    poly([(36, 34), (36, 56), (70, 34)], "url(#wood)", 1.6)
    poly([(W - 36, 34), (W - 36, 56), (W - 70, 34)], "url(#wood)", 1.6)
    # candles in iron cups on the back wall
    for cx in (175, 305):
        add(f'<circle cx="{cx}" cy="58" r="20" fill="#fff"/>')
        add(f'<path d="M{cx-10},74 L{cx+10},74 L{cx+6},80 L{cx-6},80 Z" fill="#000"/>')
        add(f'<rect x="{cx-2}" y="80" width="4" height="10" fill="#000"/>')
        candle(cx, 74)
    # barrels, right
    for (bx, by, bw, bh) in ((350, 112, 40, 52), (392, 120, 38, 48)):
        add(f'<rect x="{bx}" y="{by}" width="{bw}" height="{bh}" rx="12" fill="url(#wood)" stroke="#000" stroke-width="2.5"/>')
        for f_ in (.22, .78):
            line(bx + 2, by + bh * f_, bx + bw - 2, by + bh * f_, 3)
    add('<ellipse cx="370" cy="113" rx="18" ry="4" fill="#fff" stroke="#000" stroke-width="2"/>')
    # a ladder leaning on the left wall
    rails = ((100, 40, 76, H), (126, 40, 102, H))
    for (x1, y1, x2, y2) in rails:
        line(x1, y1, x2, y2, 3.5)
    for k in range(1, 9):
        t = k / 9
        y = 40 + (H - 40) * t
        line(100 + (76 - 100) * t, y, 126 + (102 - 126) * t, y, 2.5)

elif kind == "crypt":
    FY = 136
    bricks(0, 0, W, FY, course=18, marks=True)
    floor(FY, 240, 40)
    # three arches stepping back toward a small lit window
    # the far end first: a small window
    arch_window(240, 54, 30, 46, bars=False)
    line(240, 54, 240, 100, 2.2)
    # the arches, back to front: piers and a round head each
    for (l, r, top, base, pw) in ((172, 308, 40, FY + 2, 12), (110, 370, 18, FY + 14, 20), (34, 446, -6, H, 32)):
        cx, rad = (l + r) / 2, (r - l) / 2
        spring = top + rad
        # the arch head as a thick band
        add(f'<path d="M{l-pw},{spring} A{rad+pw},{rad+pw} 0 0 1 {r+pw},{spring} L{r},{spring} A{rad},{rad} 0 0 0 {l},{spring} Z" fill="url(#dots)" stroke="#000" stroke-width="2"/>')
        # voussoir joints
        for k in range(1, 9):
            a = math.pi + k * math.pi / 9
            line(cx + rad * math.cos(a), spring + rad * math.sin(a), cx + (rad + pw) * math.cos(a), spring + (rad + pw) * math.sin(a), 1.4)
        for x in (l - pw, r):
            add(f'<rect x="{x:.1f}" y="{spring:.1f}" width="{pw}" height="{base-spring:.1f}" fill="#fff" stroke="#000" stroke-width="2"/>')
            yy = spring + 14
            while yy < base - 4:
                line(x, yy, x + pw, yy, 1.2)
                yy += 14
            add(f'<rect x="{x-3:.1f}" y="{spring-2:.1f}" width="{pw+6}" height="6" fill="#fff" stroke="#000" stroke-width="2"/>')
    # rubble and a skull at the foot of the near right pier
    for (rx, ry, rw, rh) in ((392, H - 12, 14, 8), (410, H - 9, 10, 6), (60, H - 10, 12, 7)):
        add(f'<rect x="{rx}" y="{ry}" width="{rw}" height="{rh}" rx="2" fill="#fff" stroke="#000" stroke-width="2"/>')

else:  # hall
    print("hall is drawn by draw_scene.py", file=sys.stderr)
    sys.exit(1)

add('</svg>')
open(out, "w").write("\n".join(o))
