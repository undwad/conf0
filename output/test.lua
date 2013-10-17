require 'std'
require 'conf0'

--debug.sethook(function (event, line) print(debug.getinfo(2).short_src .. ":" .. line) end, "l")

function savestack() conf0.savestack('f:/github/conf0/output/stack.txt') end

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

port2opaque = opaque2port

function execute(params)
	local pref =  params.ref
	local ref
	local callback = params.callback
	params.callback = function(res)
		res.ref = ref
		if callback(res) then
			ref = nil
		end
	end
	ref = params.proc(params)
	if not pref or 'avahi' ~= conf0.backend then
		while ref and conf0.iterate{ref=ref} do end
	end
end

print(prettytostring(conf0))

--local registrator = conf0.register_{type = "_http._tcp", name = 'conf0test', port = port2opaque(5500), callback = function(res) print(prettytostring(res)) end}
--conf0.iterate{ref=registrator}

---[[

local items = {}

-- _http._tcp _rtsp._tcp
execute{proc = conf0.browse, type = '_rtsp._tcp', callback = function(i) 
	io.write('.')
	if i.name and i.type and i.domain and not items[i.name] then
		items[i.name] = i
		execute{proc = conf0.resolve, ref = i.ref, interface_ = i.interface_, protocol = i.protocol, name = i.name, type = i.type, domain = i.domain, callback = function(j)
			io.write('.')
			for k,v in pairs(j) do i[k] = v end
			i.port = opaque2port(i.opaqueport)
			if j.hosttarget then
				execute{proc = conf0.query, ref = j.ref, fullname = j.hosttarget, type = conf0.types.A, class_ = conf0.classes.IN, callback = function(k)
					io.write('.')
					i.ip = inet_ntoa(k.data)
					return true
				end}
			end
			return true
		end}
	end
end}

print(prettytostring(items))	

--]]