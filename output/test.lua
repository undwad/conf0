require 'std'
require 'conf0'

print(prettytostring(conf0))

if 'bonjour' == conf0.backend then
	
	byte = string.byte

	function inet_ntoa(addr) return byte(addr[1])..'.'..byte(addr[2])..'.'..byte(addr[3])..'.'..byte(addr[4]) end

	function tobytes(value, num)
		local bytes = ""
		num = num or 4
		for i = 1, num do
			bytes = string.char(value % 256) .. bytes
			value = math.floor(value / 256)
		end
		return bytes
	end

	function opaque2port(port)
		local bytes = tobytes(port)
		return byte(bytes, 4) * 256 + byte(bytes, 3)
	end

else 
	function opaque2port(port) return port end
end

port2opaque = opaque2port

function execute(params)
	local ref
	local callback = params.callback
	params.callback = function(res)
		if callback(res) then
			ref = nil
		end
	end
	ref = params.proc(params)
	if not params.ref then
		while ref and conf0.iterate{ref=ref} do end
	end
	return ref
end

local type = '_rtsp._tcp'
--local type = '_http._tcp'
local port = 5500
local name = 'conf0test'

execute{proc = conf0.connect, callback = function(res)
	execute{proc = conf0.register_, ref = res.ref, type = type, name = name, port = port2opaque(port), callback = function(res) print('registering', prettytostring(res)) end}
	local items = {}
	execute{proc = conf0.browse, ref = res.ref, type = type, callback = function(i) 
		if i.name and i.type and i.domain and not items[i.name] then
			items[i.name] = i
			execute{proc = conf0.resolve, ref = res.ref, interface_ = i.interface_, protocol = i.protocol, name = i.name, type = i.type, domain = i.domain, callback = function(j)
				for k,v in pairs(j) do i[k] = v end
				if i.opaqueport then
					i.port = opaque2port(i.opaqueport)
				end
				if i.host then
					i.addrs = conf0.gethostbyname{name = i.host}
				end
				if 'bonjour' == conf0.backend then
					if j.host then
						execute{proc = conf0.query, ref = res.ref, fullname = j.host, type = conf0.types.A, class_ = conf0.classes.IN, callback = function(k)
							i.ip = inet_ntoa(k.data)
							print(prettytostring(i))
							return true
						end}
					end
				else
					print(prettytostring(i))
				end
				return true
			end}
		end
		if 'avahi' == conf0.backend and conf0.events.BROWSER_ALL_FOR_NOW == i.event_ then
			conf0.disconnect{ref=res.ref}
		end
	end}
end}

	

