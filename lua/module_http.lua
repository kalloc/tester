#!/usr/bin/lua


module={
    type="Tcp",
    name="HTTP",
    id=3
}

function main(argv) 


    local charset=nil
    local net=api:connect()
    local r

    local get = function (path,arg) 
	local Request=[[GET /%s?%s HTTP/1.1
Host: %s
User-Agent: Module_HTTP.Lua (SoloTester.Ru)

]]
	net:write(Request:format(path,arg,argv.host))
	r=net:read() or ""
	return r
    end

    local authget = function (path,login,password) 
	local Request=[[GET /%s HTTP/1.1
Host: %s
User-Agent: Module_HTTP.Lua (SoloTester.Ru)
Authorization: Basic %s

]]
	net:write(Request:format(path,argv.host,(login..":"..password):base64encode()))
	r=net:read() or ""
	return r
    end

    local post = function (path,arg) 

	local Request=[[POST /%s HTTP/1.1
Host: %s
User-Agent: Module_HTTP.Lua (SoloTester.Ru)
Content-Length: %d
Content-Type: application/x-www-form-urlencoded

%s]]
	net:write(Request:format(path,argv.host,#argv.data[2],argv.data[2]))
	r=net:read() or ""
	return r
    end
    
    
    if argv.generror then net:error() return end

    if argv.method == 'get' then

        r=get(argv.data[1],argv.data[2])
        if r:find('200 OK') then return net:done() end

    elseif argv.method == 'auth' then

        r=authget(argv.data[1],argv.data[2],argv.data[3])
        if r:find('200 OK') then return net:done() end

    elseif argv.method == "post" then

	r=post(argv.data[1],argv.data[2])
        if r:find('200 OK') then return net:done() end

    elseif argv.method == "chkwords" then

	r=get(argv.data[1],"")
	if r then
	    charset=r:gmatch('Content%-Type:[ ]+[^;]+;[ ]+charset=([a-zA-Z%-0-9]+)')()
	    if charset and not charset:gfind('([Uu][Tt][Ff]%-?8)')() then
		r=r:iconv(charset:upper()) or r
	    end
	    if r and ((argv.data[2] == "1" and r:find(argv.data[3])) or (argv.data[2] == "0" and not r:find(argv.data[3]))) then    return net:done() end
	end

    elseif argv.method == "chksize" then

	r=get(argv.data[1],"")
	if not r then return net:error() end
	local finded,offset=r:find("\r?\n\r?\n")
	if offset and (#r-offset) == tonumber(argv.data[2]) then  return  net:done() end
    end


    net:error()
end

