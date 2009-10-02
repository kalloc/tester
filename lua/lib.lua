#!/usr/bin/lua
require "socket"
require "base64"
require "md5"
crypto = require("crypto")
evp = require("crypto.evp")
hmac = require("crypto.hmac")

function clone(t)            -- return a copy of the table t
  local new = {}             -- create a new table
  local i, v = next(t, nil)  -- i is an index of t, v = t[i]
  while i do
    new[i] = v
    i, v = next(t, i)        -- get next index
  end
  return new
end



function unPackData(packed) 
    local i,len,pak,list,razr,data;
    data={}
    razr=packed:sub(0,1);
    packed=packed:sub(2);
    list=packed:sub(0,1);
    pak=packed:sub(razr+1);
    for i=0,list-1 do
	razr=pak:sub(0,1);
        len=pak:sub(2,razr+1);
	pak=pak:sub(razr+2);
        table.insert(data,pak:sub(0,len))
        pak=pak:sub(len+1)
    end
    list=data[1]
    table.remove(data,1)
    return list,data
end


_G.api={}
function api:read (format)
	self.sock:settimeout(10)
	local data=""
	local ret=""
	do  ret=self.sock:receive(1) while ret do
	    data=data..ret
	    self.sock:settimeout(0.1)
	    ret=self.sock:receive(1)
	    end
	end
	if data ~= nil then
		print(" < "..data:gsub("\n\n|\r\n\r\n","\n"):gsub("\n","\n < "):gsub("\n <",""):sub(0,512))
	end
	return data
end

function api:write (data)
	print(" > "..data:gsub("\n\n","\n"):gsub("\n","\n < "):sub(0,512))
	return self.sock:send(data)
end
function api:finish (result)
	if type(result) ~= 'table' then
		result={state=result}
	end
	self.sock:close()	
end
function api:b64encode (s)
	return base64.encode(s)
end
function api:b64decode(s)
	return base64.decode(s)
end
function api:done(s)
	print("*	 результат [ 'программа завершила выполнения указав результат работы — OK' ]")
	self:finish(1)
end
function api:close() end
function api:error(code)
	print("*	 результат [ 'программа завершила выполнения указав результат работы — ERROR"..(code and " ["..code.."]" or "").."' ]")
	self:finish(0)
end
function api:connect(host,port) 
	local o={}
	print(host)
	o.sock=socket.connect(host or argv.host,port or argv.port)
	o.sock:settimeout(1)
	setmetatable(o, self)
	self.__index = self
	return o

end
_G.run=function(file,argv)
	local errtxt
	local func,err=loadfile(file)
	if func == nil then
		print("Error: "..err)
		return -1
	end
	string.base64encode=function (s)
	    return base64.encode(s)
	end
	string.base64decode=function (s)
	    return base64.decode(s)
	end
	string.hash=function (algo,s)
	    return evp.digest(algo,s)
	end
	string.hmac=function (algo,key,data)
	    return hmac.digest(algo,data,key);
	end
	local err,errtxt=pcall(func)
	if not err then
		print("Error: "..errtxt)
		return -1
	end
	local env=getfenv(main)
	setmetatable(env, {__index={api=_G.api,argv=argv},__newindex=function (k2,k) error("используй  local для создания переменных "..tostring(k)) end})
	setfenv(main,env)
	local ret=main(argv)
	local env=getfenv(func)
	return 1
	--    for k,v in pairs(env) do print(k,v) end
end


