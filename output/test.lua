require 'std' -- load lua standard library for pretty printing etc.

pprint = function(arg) print(prettytostring(arg)) end

browse = require 'browse0conf' -- load browse function from browse0conf module

pprint(conf0) -- print conf0 module to see what we have


local type = '_rtsp._tcp'
--local type = '_http._tcp'
local port = 5500 -- port of service to register
local name = 'conf0test' -- name of service to register

list = {}

debug.sethook(function (event, line) print(debug.getinfo(2).short_src .. ":" .. line) end, "l")

while true do
	browse{type = type, callback = function(svc)
		pprint(svc)
		if svc.new then
			if svc.ip then
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
end

