require 'std'
require 'conf0'

print(conf0.test{p1=true, p2=123.321, p3=-123, p4=123, p5='JODER', p6={}, p7=function() end, p8=true, p12='FUCK'})

browser = conf0.browse{type='_http._tcp', callback=function(item)
	print(item)
end}
conf0.iterate{ref=browser}