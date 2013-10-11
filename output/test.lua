require 'conf0'
require 'std'

function string.starts(text, start) return string.sub(text, 1, string.len(start)) == start end
function string.ends(text, finish) return finish == '' or string.sub(text, -string.len(finish)) == finish end

byte = string.byte

function inet_ntoa(addr) return byte(addr[1])..'.'..byte(addr[2])..'.'..byte(addr[3])..'.'..byte(addr[4]) end

function conf0.once(func, ...)
	local args = {...}
	local client, error, code = func(table.unpack(args))
	if client then
		local result, error, code = conf0.handle(client)
		if nil == result then print(error, code) end
		conf0.stop(client)
	else print(error, code)	end
end

local items = {}

local browser, error, code = conf0.browse(function(item) 
	if string.starts(item.name, 'AXIS') then
		items[#items + 1] = item	
		conf0.once(conf0.resolve, item.name, item.type, item.domain, function(res) 
			item.fullname = res.fullname
			item.hosttarget = res.hosttarget
			item.opaqueport = res.opaqueport
			local bytes = conf0.bytes(res.opaqueport)
			item.port = byte(bytes, 1) * 256 + byte(bytes, 2)
			conf0.once(conf0.query, item.hosttarget, 1, function(res) 
				item.ip = inet_ntoa(res.data)
			end) 
		end) 
	end
end, "_http._tcp", "local.")

if browser then
	local handle = true
	while handle do
		local handle, error, code = conf0.handle(browser)
		if not handle then
			if nil == handle then 
				print(error, code) 
			end
			break
		end
	end
	conf0.stop(browser)
else
	print(error, code)
end

print(prettytostring(items))
