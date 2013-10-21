require 'std' -- load lua standard library for pretty printing etc.
browse = require 'browse0conf' -- load browse0conf module

print(prettytostring(conf0)) -- print conf0 module to see what we have


local type = '_rtsp._tcp'
--local type = '_http._tcp'
local port = 5500 -- port of service to register
local name = 'conf0test' -- name of service to register

browse{type = type, callback = function(svc)

end}

