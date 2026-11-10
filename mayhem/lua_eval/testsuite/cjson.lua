local d = cjson.decode('{"a":[1,2,{"b":null}],"c":"str","d":1.5e3,"e":true}')
return cjson.encode({d.a[2], d.c, d.d, d.e, nested = {1, 2, 3}})
