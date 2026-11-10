local t = {}
for w in string.gmatch('the quick brown fox', '%a+') do t[#t + 1] = w:upper() end
return string.format('%s|%5.2f|%q', table.concat(t, ','), 3.14159, 'a"b') .. string.rep('ab', 3, '-')
