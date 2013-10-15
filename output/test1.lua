require 'std'
require 'conf0'

function savestack() conf0.savestack('f:/github/conf0/output/stack.txt') end

print(conf0.test{p1=true, p2=123.321, p3=-123, p4=123, p5='JODER', p6={}, p7=function() end, p8=true, p12='FUCK'})

local browser = conf0.browse{type='_http._tcp', callback=function(i)
	--print('browser says ', i)
	local resolver = conf0.resolve{name = i.name, type = i.type, domain = i.domain, callback=function(j)
		--print('resolver says ', j)
	end}
	conf0.iterate{ref=resolver}
end}

savestack()

while true do 
	print('browser loop', conf0.iterate{ref=browser})
end