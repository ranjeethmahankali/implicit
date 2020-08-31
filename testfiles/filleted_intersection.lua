l = 2
setbounds(-5, -5, -5, 5, 5, 5)
s1 = sphere(-l, 0, 0, l+1)
s2 = sphere(l, 0, 0, l+1)
i1 = bintersect(s1, s2)
i2 = filleted_intersection(s1, s2, 0.1)

-- h1 = halfspace(0, 0, 0, 0, 0, -1)
-- h2 = halfspace(0, 0, 0, 0, -1, 0)
-- hi = bintersect(h1, h2)
-- hf = filleted_intersection(h1, h2, 0.5)