require 'std'

pprint = function(arg) print(prettytostring(arg)) end

browse = require 'browse0conf'

list = {}

while true do
	browse{type = '_rtsp._tcp', callback = function(svc)
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