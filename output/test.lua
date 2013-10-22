require 'std' -- load lua standard library for pretty printing etc.

pprint = function(arg) print(prettytostring(arg)) end

browse = require 'browse0conf' -- load browse function from browse0conf module

pprint(conf0) -- print conf0 module to see what we have

local type = '_rtsp._tcp'
--local type = '_http._tcp'

--[[ custom service registration example

connection = conf0.execute{ -- begin calling conf0.connect 
	proc = conf0.connect, -- service function to call
	callback = function(connection) -- required conf0.connect callback
		registrator = conf0.execute{ -- begin calling conf0.register_ 
			proc = conf0.register_, -- service function to call
			ref = connection.ref, -- service reference
			type = type, -- service type
			name = 'conf0test', -- service name
			port = port2opaque(5500), -- service port
			texts = {'sample text', 'key=value'}, -- service texts
			callback = function(registered) -- begin register callback
				pprint(registered) -- print result
			end -- end register callback
		} -- end calling conf0.register_ 
	end
}

--]]
---[[

list = {}

while true do
	local s,e = pcall(function()
		browse{type = type, callback = function(svc)
			if svc.new then
				if svc.ip then
					svc.ref = nil
					list[svc.name] = svc
					svc.index = table.size(list)
					pprint(svc)
				else return not list[svc.name] end
			elseif svc.remove then
				list[svc.name] = nil
				print('service '..svc.name..' removed')
			elseif svc.error then
				print('resolving service '..svc.name..' failed')
			end
		end}
	end)
	if not s then print(e) end
	collectgarbage()
end

--]]