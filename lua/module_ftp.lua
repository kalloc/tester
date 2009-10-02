#!/usr/bin/lua

local net

module={
	type="Tcp",
	name="FTP",
	id=5
}

local dump =function(t,pad)
    if not pad then pad = 0 end
    for k,v in pairs(t) do
	
	if type(v) == "table" then
	    print(("\t"):rep(1).."table "..k)
	    dump(v,pad+1)
	else 
	    print(("\t"):rep(1)..k..'=>'..tostring(v))
	end
    end

end


function main(argv) 
--	dump(argv)

	local net,pasv,ret,ret2


	local login=function() 
		--читаем заголовок демона
		ret=net:read()
		if not ret or not ret:find('^220') then return nil	end

		--пишем в буфер логин
		net:write(("USER %s\n"):format(argv.data[1] or "anonymous@anon"))
		ret=net:read()
		if not ret or not ret:find('^331') then return nil	end

		--пишем в буфер пароль
		net:write(("PASS %s\n"):format(argv.data[2] or "guest"))

		ret=net:read()
		if not ret or not ret:find('^230') then return nil	end
		return 1
	end

	local passive = function()
		--включаем пасивный режим
		net:write("EPSV\n")
		local port=string.match(net:read() or "","\|||(%d+\)|")
		if port == nil then
			return nil
		end
		pasv=api:connect(argv.ip,port)
		return pasv
	end


	local list = function(path) 
		--запрашиваем список файлов
		pasv=passive(net)

		if not pasv then return nil end
		if path == "" then
		    path="/"
		elseif path:sub(-1,-1) ~= '/' then 
		    path=path.."/" 
		end

	        net:write(("LIST %s\n"):format(path))
		--читаем из него и возвращаем результат работы
		ret=pasv:read();    
		
		pasv:close()
		net:read()
		if not ret or #ret == 0 then return nil end
		return ret

	end

	local content = function (path,file) 
		--запрашиваем содермимое файла
		local pasv=passive()
		if not pasv  then 
		    net:error()
		    return nil
		end
		if path:sub(-1,-1) ~= '/' then path=path.."/" end


		net:write(("RETR %s%s\n"):format(path,file))
		ret=pasv:read()
		pasv:close()
		ret2=net:read()
		if not ret2 or ret2:find("^550") then return nil end
		--читаем из него и возвращаем результат работы
		return ret
    	end

	local store = function (path,file) 
		--запрашиваем содермимое файла
		pasv=passive()
		
		if not pasv  then 
		    net:error()
		    return nil
		end
		if path:sub(-1,-1) ~= '/' then path=path.."/" end

	        net:write(("STOR %s%s\n"):format(path,file))

		ret=pasv:write("This Is Simple Content") or ""
		pasv:close()
		ret=net:read()
		return ret and ret:find('^150') or nil
    	end
    	
    	local cwd = function(path) 
	    if path:sub(-1,-1) ~= '/' then path=path.."/" end

	    net:write("CWD "..path.."\n")
	    ret=net:read()
	    return ret and ret:find('^250') or nil
    	end


    	local stat = function(path,file) 
	    if path:sub(-1,-1) ~= '/' then path=path.."/" end

	    net:write(("STAT %s%s\n"):format(path,file))
	    ret=net:read()
	    return ret and ret:find(file) or nil
    	end

    	local delete = function(path,file) 
	    if path:sub(-1,-1) ~= '/' then path=path.."/" end

	    net:write(("DELE %s%s\n"):format(path,file))
	    ret=net:read()
	    return ret and ret:find('^250') or nil
    	end
    	
    	local logout = function(flag,cdoe)
    	    net:write("QUIT\n")
    	    if flag then
    		return net:done(code)
    	    else
    		return net:error(code)
    	    end
    	end

	net=api:connect()

	if argv.generror then return logout(nil,"GENERIC")  end

	--проверка доступа
	if not login()  then return logout(nil,"LOGIN") end

	if argv.method == "auth" then 
	    return net:done()

	elseif argv.method == "cwd" then
	-- попытка сменить директорию
	    if not cwd(argv.data[3]) then return logout(nil,"CWD")	end

	elseif argv.method == "chkfile" then
	-- есть ли файл
	    if not stat(argv.data[3],argv.data[4]) then return logout(nil,"CHKFILE")	end

	elseif argv.method == "list" then

	--проверка списка файлов
	    if not list(argv.data[3])  then return logout(nil,"LIST")  end
	elseif argv.method == "put" then
	--закачка файда
	    if not store(argv.data[3],argv.data[4])  then return logout(nil,"PUT")  end

	elseif argv.method == "delete" then
	-- удаление файла
	    if not delete(argv.data[3],argv.data[4])  then return logout(nil,"DELETE")  end

	elseif argv.method == "retr" then

	--проверка содержимого файласписка файлов
	    if not content(argv.data[3],argv.data[4]) then  return logout(nil,"RETR") end
	end
	
	return logout(1,"")

end
