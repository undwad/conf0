require 'conf0' -- load conf0 module
 
-- tests if flag is set
function testflag(set, flag) return set % (2 * flag) >= flag end

-- we the following only on bonjour since avahi does not use opaque port and resolves service ip
if 'bonjour' == conf0.backend then
	
	-- converts address as integer to string in the form of ip
	function inet_ntoa(addr) return string.byte(addr[1])..'.'..string.byte(addr[2])..'.'..string.byte(addr[3])..'.'..string.byte(addr[4]) end

	-- converts integer to byte array
	function tobytes(value, num)
		local bytes = ""
		num = num or 4
		for i = 1, num do
			bytes = string.char(value % 256) .. bytes
			value = math.floor(value / 256)
		end
		return bytes
	end

	-- converts opaque value to port
	function opaque2port(port)
		local bytes = tobytes(port)
		return string.byte(bytes, 4) * 256 + string.byte(bytes, 3)
	end

	-- converts port to opaque value
	port2opaque = opaque2port

elseif 'avahi' == conf0.backend then

else -- unknown backend value
	error('invalid conf0 backend '..conf0.backend)
end

-- executes service function and wait for callback
function conf0.execute(params)
	local ref -- variable for service reference
	local callback = params.callback -- saves callback passed by user
	params.callback = function(res) -- new local callback proc
		if callback(res) then -- if user's callback returns true 
			ref = nil -- then we don't need service reference anymore
		end
	end
	ref = params.proc(params) -- executes service function
	if not params.ref then -- if succeeded and while 
		while ref and conf0.iterate{ref=ref} do end
	end
	return ref -- returns service reference
end

return function(params)
	-- parameters check
	if 'table' ~= type(params) then error('function only accepts single parameter of type table') end
	if 'function' ~= type(params.callback) then error('callback named parameter missing') end
	local callback = params.callback -- saves user callback since we will use params as conf0.browse argument
	conf0.execute{ -- begin calling conf0.connect 
		proc = conf0.connect, -- service function to call
		callback = function(connection) -- required conf0.connect callback
			-- checks connection state
			if connection and ('bonjour' == conf0.backend or ('avahi' == conf0.backend and conf0.states.CLIENT_S_RUNNING == connection.state)) then
				params.proc = conf0.browse -- service function to call
				params.ref = connection.ref -- service reference
				params.callback = function(browsed) -- callback for browser
					if 'bonjour' == conf0.backend then -- checks bonjour flags
						browsed.new = testflag(browsed.flags, conf0.flags.Add) -- new service
						browsed.remove = not browsed.new -- removed service
					elseif 'avahi' == conf0.backend then -- checks avahi event
						if conf0.events.BROWSER_CACHE_EXHAUSTED == browsed.event_ then -- almost done
							return false -- stop callback execution but not connection iteration
						elseif conf0.events.BROWSER_ALL_FOR_NOW == browsed.event_ then -- done
							conf0.disconnect{ref = connection.ref} -- on avahi we need to stop iteration manually
							return true -- stop callback execution and connection iteration
						else -- service browsed
							browsed.new = conf0.events.BROWSER_NEW == browsed.event_ -- new service
							browsed.remove = conf0.events.BROWSER_REMOVE == browsed.event_ -- removed service
							browsed.error = conf0.events.BROWSER_FAILURE == browsed.event_ -- service browsing error
						end
					end
					if callback(browsed) then
						browsed.proc = conf0.resolve -- service function to call
						browsed.ref = connection.ref -- service reference
						browsed.callback = function(resolved) -- callback for browser
							for k,v in pairs(resolved) do browsed[k] = v end -- copy all fields of resolve result into browse result
							if 'bonjour' == conf0.backend then -- on bonjour
								browsed.port = opaque2port(browsed.opaqueport) -- we need to decode opaque port value
								-- now we need to query for ip address
								conf0.execute{ -- begin calling conf0.query 
									proc = conf0.query, -- service function to call
									fullname = browsed.host, -- record name is host name for this request
									type = conf0.types.A, -- record type for address (see conf0.types)
									class_ = conf0.classes.IN, -- record class (see conf0.classes)
									callback = function(record) -- begin query callback
										browsed.ip = inet_ntoa(record.data) -- converts ip address to string
										browsed.error = record.error -- pass possible record error to user callback
										callback(browsed) -- now we are done with this service
										return true -- returns true to tell execute method that the appropriate callback was successfuly called
									end -- end query callback
								} -- end calling conf0.query 
							elseif 'avahi' == conf0.backend then -- on avahi
								browsed.error = conf0.events.RESOLVER_FAILURE == browsed.event_ -- we need to check for resolve error
								callback(browsed) -- since avahi resolves ip address now we are done with this service
							end
							return true -- returns true to tell execute method that the appropriate callback was successfuly called
						end
						conf0.execute(browsed) -- calls resolver
					end
				end
				conf0.execute(params) -- calls browser
			end
		end
	} -- end calling conf0.connect 
end

--		execute{ -- begin calling conf0.register_ 
--			proc = conf0.register_, -- service function to call
--			ref = res.ref, -- service reference
--			type = type, -- service type
--			name = name, -- service name
--			port = port2opaque(port), -- service port
--			callback = function(res) -- begin register callback
--				print('registering', prettytostring(res)) -- print result
--				-- check res.flags with conf0.flags on bonjour
--				-- check res.state with conf0.state on avahi
--			end -- end register callback
--		} -- end calling conf0.register_ 
--]]

-- many functions have flags, interface, protocol etc. parameters that are set to default values
-- check this in bonjour / avahi documentations and in conf0.flags, conf0.interfaces, conf0.protocols etc.