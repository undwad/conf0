require 'conf0'
require 'std'

byte = string.byte

function inet_ntoa(addr) return byte(addr[1])..'.'..byte(addr[2])..'.'..byte(addr[3])..'.'..byte(addr[4]) end

function port2opaque(port)
	local bytes = conf0.bytes(port)
	return byte(bytes, 1) * 256 + byte(bytes, 2)
end

function conf0.once(func, ...)
	local args = {...}
	local client, error, code = func(table.unpack(args))
	if client then
		local result, error, code = conf0.iterate(client)
		if nil == result then print(error, code) end
	else print(error, code)	end
end

function conf0.wait(func, ...)
	local args = {...}
	local client, error, code = func(table.unpack(args))
	if client then
		local result = true
		while result do
			local result, error, code = conf0.iterate(client)
			if not result then
				if nil == result then print(error, code) end
				break
			end
		end
	else print(error, code)	end
end

common = conf0.common()

conf0.once(conf0.enumdomain, common, function(res) print(res) end) 

--local me, error, code = conf0.register("_http._tcp", function(res) print(prettytostring(res)) end, 'conf0test', nil, port2opaque(5500))
--if me then conf0.handle(me) else print(error, code) end

local items = {}

conf0.wait(conf0.browse, common, function(item) 
	items[#items + 1] = item	
	--conf0.once(conf0.resolve, item.name, item.type, item.domain, function(res) 
	--	item.fullname = res.fullname
	--	item.hosttarget = res.hosttarget
	--	item.opaqueport = res.opaqueport
	--	item.port = port2opaque(res.opaqueport)
		--conf0.once(conf0.query, item.hosttarget, 1, function(res) 
		--item.ip = inet_ntoa(res.data)
		--end) 
	--end) 
end, "_http._tcp", "local.")

print(prettytostring(items))

os.execute('pause')

conf0.stop(me)