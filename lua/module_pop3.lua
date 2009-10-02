#!/usr/bin/lua

module={
    type="Tcp",
    name="POP3",
    id=7
}




function main(argv) 
    local net=api:connect()
    local r;

    --правильно ли работает сервер
    r=net:read()
    if  not r or not r:find("^+OK") then return net:error()  end
    if #argv.data < 2 then return net:error() end
    --авторизовываемся
    net:write("USER "..argv.data[1].."\n")
    r=net:read()
    if  not r or not r:find("^+OK") then return net:error()  end


    net:write("PASS "..argv.data[2].."\n")
    r=net:read()
    if  not r or not r:find("^+OK") then return net:error()  end

    --NOOP
    if argv.method == "noop" then
	net:write("NOOP\n")

	r=net:read() or ""
	if not r:find("^+OK") then   net:error()   return end
    --LIST
    elseif argv.method == "list" then

	net:write("LIST\n")

	r=net:read() or ""
	if not r:find("^+OK") then   net:error()   return end
    --RETR
    elseif argv.method == "retr" then
        if #argv.data < 4 then return net:error() end


	net:write("STAT\n")
	r=net:read() or ""
	if not r:find("^+OK") then   net:error()   return end

	local num=r:match("+OK (%d+)")
	local found=nil
	local i

	for i = 1,num do
	    net:write("TOP "..i.."\n")	
	    r=net:read() or ""
    	    if not r:find("^+OK") then   net:error()   return end
    	    if r:find(":.+<"..argv.data[3]..">") and r:find("Subject: "..argv.data[4]) then
    		found=i
    		break
    	    end
	end
	if not found then return net:error() end
	
	net:write("RETR "..found.."\n")
	r=net:read() or ""
	if not r:find("^+OK") then   net:error()   return end

    --DELETE
    elseif argv.method == "delete" then
        if #argv.data < 4 then return net:error() end

	net:write("STAT\n")
	r=net:read() or ""
	if not r:find("^+OK") then   net:error()   return end

	local num=r:match("+OK (%d+)")
	local found=nil
	local i

	for i = 1,num do
	    net:write("TOP "..i.."\n")	
	    r=net:read() or ""
    	    if not r:find("^+OK") then   net:error()   return end
    	    if r:find(":.+<"..argv.data[3]..">") and r:find("Subject: "..argv.data[4]) then
    		found=i
    		break
    	    end
	end
	if not found then return net:error() end
	
	net:write("DELE "..found.."\n")
	r=net:read() or ""
	if not r:find("^+OK") then   net:error()   return end
    --CHECK WORDS
    elseif argv.method == "chkwords" then

        if #argv.data < 6 then return net:error() end
	net:write("STAT\n")
	r=net:read() or ""
	if not r:find("^+OK") then   net:error()   return end

	local num=r:match("+OK (%d+)")
	local found=nil
	local i

	for i = 1,num do
	    net:write("TOP "..i.." 0\n")	
	    r=net:read() or ""
    	    if not r:find("^+OK") then   net:error()   return end
    	    if r:find(":.+<"..argv.data[3]..">") and r:find("Subject: "..argv.data[4]) then
    		found=i
    		break
    	    end
	end
	if not found then return net:error() end
	
	net:write("RETR "..found.."\n")
	r=net:read() or ""
	if not r:find("^+OK") or ((argv.data[5] == "1"  and not r:find(argv.data[6])) or (argv.data[5] == "0" and r:find(argv.data[6])))  then   net:error()   return end

    end

    --выходим
    net:write(string.format("QUIT\n"))

    if argv.generror then net:error() return end
    net:done()
end

