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
	params.callback = function(browsed)
	end
	conf0.execute{ -- begin calling conf0.connect 
		proc = conf0.connect, -- service function to call
		callback = function(connection) -- required conf0.connect callback
			-- checks connection state
			if connection and ('bonjour' == conf0.backend or ('avahi' == conf0.backend and conf0.states.CLIENT_S_RUNNING == connection.state)) then
				print(connection)
			end
		end
	} -- end calling conf0.connect 
end

--[[
execute{ -- begin calling conf0.connect 
	proc = conf0.connect, -- service function to call
	callback = function(res) -- required conf0.connect callback
		local items = {} -- table for browsable services
		execute{ -- begin calling conf0.browse 
			proc = conf0.browse, -- service function to call
			ref = res.ref, -- service reference
			type = type, -- service type
			callback = function(i) -- begin browse callback
				-- if browse result is valid service and yet isn't reported 
				if i.name and i.type and i.domain and not items[i.name] then 
					items[i.name] = i -- then store its info in the table
					execute{ -- begin calling conf0.resolve 
						proc = conf0.resolve, -- service function to call
						ref = res.ref, -- service reference
						interface_ = i.interface_, -- service interface
						protocol = i.protocol, -- service protocol
						name = i.name, -- service name
						type = i.type, -- service type
						domain = i.domain, -- service domain
						callback = function(j) -- begin resolve callback
							for k,v in pairs(j) do i[k] = v end -- copy all fields of resolve result into query result
							if i.opaqueport then -- if has opaque port 
								i.port = opaque2port(i.opaqueport) -- then we need to decode it
							end
							-- since avahi resolves ip address
							-- we need to query service record for ip address only on bonjour
							if 'bonjour' == conf0.backend then -- on bonjour
								if j.host then
									execute{ -- begin calling conf0.query 
										proc = conf0.query, -- service function to call
										ref = res.ref, -- service reference
										fullname = j.host, -- record name is host name for this request
										type = conf0.types.A, -- record type for address (see conf0.types)
										class_ = conf0.classes.IN, -- record class (see conf0.classes)
										callback = function(k) -- begin query callback
											i.ip = inet_ntoa(k.data) -- converts ip address to string
											print(prettytostring(i)) -- we can print service info right now
											return true -- returns true to tell execute method that the appropriate callback was successfuly called
										end -- end query callback
									} -- end calling conf0.query 
								end
							else -- on avahi
								print(prettytostring(i)) -- we can print service info right now
							end
							return true -- returns true to tell execute method that the appropriate callback was successfuly called
						end -- begin resolve callback
					} -- end calling conf0.resolve 
				end
				-- on avahi we need to stop iteration manually
				if 'avahi' == conf0.backend and conf0.events.BROWSER_ALL_FOR_NOW == i.event_ then
					conf0.disconnect{ -- calling conf0.disconnect 
						ref=res.ref -- service reference
					}
				end
			end -- end browse callback
		} -- end calling conf0.browse 
	end
} -- end calling conf0.connect 

		--browsed.new = testflag(browsed.flags, conf0.flags.Add)
		--browsed.remove = not browsed.new

				--if conf0.events.BROWSER_CACHE_EXHAUSTED == browsed.event_ then
				--elseif conf0.events.BROWSER_ALL_FOR_NOW == browsed.event_ then
--					conf0.disconnect{ref = connected.ref}
	--			else
		--			browsed.new = conf0.events.BROWSER_NEW == browsed.event_
			--		browsed.remove = conf0.events.BROWSER_REMOVE == browsed.event_
			--		browsed.error = conf0.events.BROWSER_FAILURE == browsed.event_

							--browsed.error = conf0.events.RESOLVER_FAILURE == browsed.event_



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