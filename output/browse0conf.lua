require 'conf0'
 
local function testflag(set, flag) return set % (2 * flag) >= flag end

local function text2list(text, textlen)
	if not textlen then textlen = string.len(text) end
	local list = {}
	local i = 1
	while i <= textlen do
		local len = string.byte(text[i])
		if len > 0 then list[#list + 1] = string.sub(text, i + 1, i + len) end
		i = i + len + 1
	end
	return list
end

if 'avahi' == conf0.backend then
elseif 'bonjour' == conf0.backend then
	local byte = string.byte

	local function inet_ntoa(addr) return byte(addr[1])..'.'..byte(addr[2])..'.'..byte(addr[3])..'.'..byte(addr[4]) end

	local function tobytes(value, num)
		local bytes = ""
		num = num or 4
		for i = 1, num do
			bytes = string.char(value % 256) .. bytes
			value = math.floor(value / 256)
		end
		return bytes
	end

	local function opaque2port(port)
		local bytes = tobytes(port)
		return byte(bytes, 4) * 256 + byte(bytes, 3)
	end

	return function(params)
		if 'table' ~= type(params) then error('function only accepts single parameter of type table') end
		if 'function' ~= type(params.callback) then error('callback named parameter missing') end
		local callback = params.callback
		params.callback = function(browsed)
			browsed.new = testflag(browsed.flags, conf0.flags.Add)
			browsed.remove = not browsed.new
			browsed.callback = nil
			if callback(browsed) then
				local resolver
				browsed.flags = nil
				browsed.callback = function(resolved)
					for k,v in pairs(resolved) do browsed[k] = v end
					browsed.list = text2list(browsed.text)
					browsed.port = opaque2port(browsed.opaqueport)		
					local query
					query = conf0.query{fullname = browsed.host, type = conf0.types.A, class_ = conf0.classes.IN, callback = function(result)
						browsed.ip = inet_ntoa(result.data)
						browsed.callback = nil
						callback(browsed)
						query = nil
					end}
					while query and conf0.iterate{ref = query} do end
					resolver = nil
				end
				resolver = conf0.resolve(browsed)
				while resolver and conf0.iterate{ref = resolver} do end
			end
			
		end
		local browser = conf0.browse(params)
		while browser and conf0.iterate{ref = browser} do end
	end 
else error('invalid conf0 backend '..conf0.backend) end

