local p = cmsgpack.pack({1, 'two', {k = 'v'}}, -5, 70000, 1.5, true)
local a, b, c = cmsgpack.unpack(p)
return {a, b, c}
