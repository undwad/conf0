require 'std'
require 'conf0'

function savestack() conf0.savestack('f:/github/conf0/output/stack.txt') end

byte = string.byte

function inet_ntoa(addr) return byte(addr[1])..'.'..byte(addr[2])..'.'..byte(addr[3])..'.'..byte(addr[4]) end

function port2opaque(port)
	local bytes = conf0.bytes(port)
	return byte(bytes, 1) * 256 + byte(bytes, 2)
end

--local me, error, code = conf0.register(common, "_http._tcp", function(res) print(prettytostring(res)) end, 'conf0test', nil, port2opaque(5500))
--if me then conf0.iterate(me) else print(error, code) end

local items = {}

-- _http._tcp _rtsp._tcp
local browser = conf0.browse{type='_http._tcp', callback=function(i) 
	items[#items + 1] = i
	io.write('.')
	local resolver = conf0.resolve{name = i.name, type = i.type, domain = i.domain, callback=function(j)
		for k,v in pairs(j) do i[k] = v end
		io.write('.')
		local query = conf0.query{fullname = j.hosttarget, type=1, class_=1, callback = function(k)
			io.write('.')
			i.ip = k.data
		end}
		conf0.iterate{ref=query}
	end}
	conf0.iterate{ref=resolver}
end}

while conf0.iterate{ref=browser} do end

print(prettytostring(items))	