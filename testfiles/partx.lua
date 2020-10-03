b = 10
setbounds(-b, -b, -b, b, b, b)
adaptive_rendermode(2)

top = box(0, 0, 7, 8, 8, 2)
bot = box(0, 0, -7, 8, 8, 2)
nds = bunion(top, bot)
w = 6
r = 1
c1 = cylinder(-w, 0, 7, w, 0, -7, r)
c2 = cylinder(w, 0, 7, -w, 0, -7, r)
fr = 1.0
cs = filleted_union(c1, c2, fr)
shape = filleted_union(cs, nds, fr)
lat = bintersect(cs, gyroid(6, .3))
part1 = linblend(lat, shape, 0, 0, 0, 0, 0, 7)
part2 = linblend(lat, shape, 0, 0, 0, 0, 0, -7)
part = bunion(bintersect(halfspace(0,0,0,0,0,1), part1), bintersect(halfspace(0,0,0,0,0,-1), part2))
