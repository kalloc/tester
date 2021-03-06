#!/usr/bin/lua
--2010-05-07 16:03

module={
	type="Tcp",
	name="TELNET",
	id=8
}


function main(argv) 
--	dump(argv)

	local net,r

	local perform=function()
	    r=net:read()
	    if not r then return nil end

	    net:write(string.char(0xff,0xfc,0x18,0xff,0xfc,0x20,0xff,0xfc,0x23,0xff,0xfc,0x27))
	    r=net:read()
	    if not r then return nil end

	    net:write(string.char(0xff,0xfe,0x03))
	    r=net:read()
	    if not r then return nil end
	    
	    net:write(string.char(0xff,0xfc,0x01,0xff,0xfc,0x1f,0xff,0xfe,0x05,0xff,0xfc,0x21))
	    r=net:read()
	    if not r then return nil end
	    return r:sub(4)
	end
	
	local login=function() 
		if not r or not r:find('login: ') then return nil	end
		net:write(argv.data[1].."\r\n")
		r=net:read()
		if not r or not r:sub(#argv.data[1]+3):find('Password:') then return nil	end
		net:write(argv.data[2].."\r\n")
		r=net:read()
		if not r or r:find('^Login incorrect') then return nil	end
		return 1
	end


	
	local cmd=function(command) 
		net:write(command.." && echo 'Module_Telnet SoloMonitoringTrue'\r\n")
		r=net:read()
		if not r or not r:find('Module_Telnet SoloMonitoringTrue') then return nil	end
		return r
	end


	net=api:connect()
	r=perform()
	
	--проверка доступа
	if not login()  then return net:error("LOGIN") end

	if argv.method == "cmd" then
	    if not cmd(argv.data[3]) then return net:error("CMD")	end
	elseif argv.method == "chkwords" then
	    r=cmd(argv.data[3])
	    if not r then return net:error('NIL') end	
	    if argv.data[4] == "1" and not r:find(argv.data[5]) then    return net:error('WORD NOT FOUND') end
	    if argv.data[4] == "0" and r:find(argv.data[5])  then    return net:error('WORD FOUND') end

	end
	net:write("exit\n")
	return net:done()

end

