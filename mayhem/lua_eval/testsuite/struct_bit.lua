local s = struct.pack('>I4 <i2 z', 0xdeadbeef, -2, 'zstr')
local a, b, c = struct.unpack('>I4 <i2 z', s)
return {bit.band(a, 0xff), bit.bxor(b, 3), bit.tohex(a), c, struct.size('>I4')}
