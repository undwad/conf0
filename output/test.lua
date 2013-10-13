require 'conf0'
require 'std'

byte = string.byte

function inet_ntoa(addr) return byte(addr[1])..'.'..byte(addr[2])..'.'..byte(addr[3])..'.'..byte(addr[4]) end

function port2opaque(port)
	local bytes = conf0.bytes(port)
	return byte(bytes, 1) * 256 + byte(bytes, 2)
end

common = conf0.common()
common = nil

os.execute('pause')

function conf0.once(func, ...)
	local args = {...}
	local client, error, code = func(table.unpack(args))
	if client then
		local result, error, code = conf0.handle(client)
		if nil == result then print(error, code) end
		conf0.stop(client)
	else print(error, code)	end
end

function conf0.wait(func, ...)
	local args = {...}
	local client, error, code = func(table.unpack(args))
	if client then
		local handle = true
		while handle do
			local handle, error, code = conf0.handle(client)
			if not handle then
				if nil == handle then print(error, code) end
				break
			end
		end
		conf0.stop(client)
	else print(error, code)	end
end

local me, error, code = conf0.register("_http._tcp", function(res) print(prettytostring(res)) end, 'conf0test', nil, port2opaque(5500))
if me then conf0.handle(me) else print(error, code) end

local items = {}

conf0.wait(conf0.browse, function(item) 
	items[#items + 1] = item	
	conf0.once(conf0.resolve, item.name, item.type, item.domain, function(res) 
		item.fullname = res.fullname
		item.hosttarget = res.hosttarget
		item.opaqueport = res.opaqueport
		item.port = port2opaque(res.opaqueport)
		--conf0.once(conf0.query, item.hosttarget, 1, function(res) 
		--item.ip = inet_ntoa(res.data)
		--end) 
	end) 
end, "_http._tcp", "local.")

print(prettytostring(items))

os.execute('pause')

conf0.stop(me)