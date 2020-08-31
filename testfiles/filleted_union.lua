l = 4
rad = 1
c1 = cylinder(-l, 0, 0, l, 0, 0, rad)
c2 = cylinder(0, l, 0, 0, -l, 0, rad)
u1 = bunion(c1, c2)
u2 = filleted_union(c1, c2, 0.5)