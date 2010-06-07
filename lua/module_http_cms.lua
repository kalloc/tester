#!/usr/bin/lua
--2010-05-07 16:03

module={
    type="Tcp",
    name="HTTP_CMS",
    id=10
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

    if argv.generror then net:error() return end

    r=get(argv.data[1],argv.data[2])
    if not r or  not r:find("Result CODE: 0") then    
	return net:error("ERROR")
    end

    return  net:done()
end

