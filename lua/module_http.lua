#!/usr/bin/lua


module={
    type="Tcp",
    name="HTTP",
    id=3
}
local tostr = function(arg,ret)
    return arg and arg ~= '' and ret or ''
end

function main(argv) 


    local charset=nil
    local net=api:connect()
    local r

    local get = function (path,arg) 
	local Request=[[GET %s%s%s%s HTTP/1.1
Host: %s
User-Agent: Module_HTTP.Lua (SoloMonitoring.Ru)

]]
	net:write(Request:format(
	    path:sub(1,1) == '/' and '' or '/',
	    path,
	    tostr(arg,'?'),
	    tostr(arg,""),
	    argv.host
	))
	return net:read() or ""
    end

    local authget = function (path,login,password) 
	local Request=[[GET %s%s HTTP/1.1
Host: %s
User-Agent: Module_HTTP.Lua (SoloMonitoring.Ru)
Authorization: Basic %s

]]
	net:write(Request:format(path:sub(1,1) == '/' and '' or '/',path,argv.host,(login..":"..password):base64encode()))
	return net:read() or ""
    end

    local post = function (path,arg) 

	local Request=[[POST %s%s HTTP/1.1
Host: %s
User-Agent: Module_HTTP.Lua (SoloMonitoring.Ru)
Content-Length: %d
Content-Type: application/x-www-form-urlencoded

%s]]
	net:write(Request:format(path:sub(1,1) == '/' and '' or '/',path,argv.host,#argv.data[2],argv.data[2]))
	r=net:read() or ""
	return r
    end
    
    
    if argv.generror then net:error() return end

    if argv.method == 'get' then

        r=get(argv.data[1],argv.data[2])
        if not r:find('200 OK') then 
    	    return net:error("GET") 
    	end

    elseif argv.method == 'auth' then

        r=authget(argv.data[1],argv.data[2],argv.data[3])
        if not r:find('200 OK') then 
    	    return net:error("AUTH")
    	end

    elseif argv.method == "post" then

	r=post(argv.data[1],argv.data[2])
        if not r:find('200 OK') then
	    return net:error("POST") 
	end

    elseif argv.method == "chkwords" then

	r=get(argv.data[1],nil)
	if r then
	    charset = argv.data[4] and argv.data[4] ~= '' and  argv.data[4] or r:gmatch('Content%-Type:[ ]+[^;]+;[ ]+charset=([a-zA-Z%-0-9]+)')()
	    if charset and not charset:gfind('([Uu][Tt][Ff]%-?8)')() then
		r=r:iconv(charset:upper()) or r
	    end
	    if not r or  not ((argv.data[2] == "1" and r:find(argv.data[3])) or (argv.data[2] == "0" and not r:find(argv.data[3]))) then    
		return net:error("CHKWORDS")
	    end
	end

    elseif argv.method == "chksize" then

	r=get(argv.data[1],nil)
	if not r then return net:error("EMPTY") end
	
	local finded,offset=r:find("\r?\n\r?\n")
	
	if not offset or (#r-offset) ~= tonumber(argv.data[2]) then  
	    return net:error("CHKSIZE") 
	end
    end

    return  net:done()
end

