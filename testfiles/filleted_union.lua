l = 4
rad = 1
c1 = cylinder(-l, 0, 0, l, 0, 0, rad)
c2 = cylinder(0, l, 0, 0, -l, 0, rad)
u1 = bunion(c1, c2)
u2 = filleted_union(c1, c2, 0.5)

uouter = offset(u2, 0.1)
shell = bsubtract(uouter, u2)
lat = bintersect(u2, gyroid(8, .2))
part1 = bunion(lat, shell)
split1 = bintersect(halfspace(0, 0, 0, 0, 0, -1), part1)
part2 = filleted_union(lat, shell, 0.15)
split2 = bintersect(halfspace(0, 0, 0, 0, 0, -1), part2)