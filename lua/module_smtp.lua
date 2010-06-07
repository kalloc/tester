#!/usr/bin/lua
--2010-05-07 16:03

module={
    type="Tcp",
    name="SMTP",
    id=4
}

function main(argv) 
    local Request=[[Envelope-to: solotester@solomonitoring.ru
From: SoloTester <solotester@solomonitoring.ru>
Reply-To: <solotester@solomonitoring.ru>
User-Agent: SoloMonitoring.Ru Module_SMTP
MIME-Version: 1.0
To: <solotester@solomonitoring.ru>
Subject: Tester Message
Content-Type: multipart/alternative;
    boundary=\"------------060501080308090602010507\"
This is a multi-part message in MIME format.
--------------060501080308090602010507
Content-Type: text/plain; charset=UTF-8
Content-Transfer-Encoding: 8bit

Это тестовое сообщение созданное для проверки работы SMTP.
LObjId %id%

--
Желаем вам удачи. SoloMonitoring.Ru
--------------060501080308090602010507

.
]]
--"
         
    local net=api:connect()
    --правильно ли работает сервер
    local ret=net:read()
    if not ret or not ret:find("^220") then  return net:error(ret) end

    net:write("EHLO solotester\n")
    
    ret = net:read()
    if not ret or not ret:find("^250") then  return  net:error(ret) end

    if ret:find("CRAM%-MD5") then
	    --авторизовываемся по CRAM-MD5
	    net:write("AUTH CRAM-MD5\n")
	    ret = net:read()
	    if not ret or not ret:find("^334") then net:error(ret) return end

	    net:write(string.base64encode(argv.data[1]..' '..string.hmac('md5',argv.data[2],ret:sub(5,-1):base64decode())).."\n")
	
    elseif ret:find("PLAIN") then
	    --авторизовываемся по PLAIN
	    net:write("AUTH PLAIN "..string.base64encode(argv.data[1]..'\0'..argv.data[1]..'\0'..argv.data[2]).."\n")

    elseif ret:find("LOGIN") then
	    --авторизовываемся по LOGIN
	    net:write("AUTH LOGIN\n")
	    ret = net:read()
	    if not ret or  not ret:find("^334") then net:error(ret) return end

	    net:write(string.base64encode(argv.data[1]).."\n")
	    ret = net:read()
	    if not ret or  not ret:find("^334") then net:error(ret) return end

	    net:write(string.base64encode(argv.data[2]).."\n")
    else
        --нечем авторизовываться
	    net:error("AUTH NOT FOUND")
	    return
    end
    ret=net:read()
    if not ret:find("^235") then net:error(ret) return end

    if argv.method == "send" then

	net:write("MAIL FROM: <solotester@solomonitoring.ru>\n");
	ret = net:read()
	if not ret or not  ret:find("^250") then       net:error(ret);       return end

	net:write("RCPT TO: <solotester@solomonitoring.ru>\n");
	ret = net:read()
	if not ret or not ret:find("^250") then       net:error(ret);       return end

	net:write("DATA\n")
	ret = net:read()
	
	if not ret or not  ret:find("^354") then       net:error(ret);       return end

	net:write(Request:gsub("%%id%%",argv.id))
	ret = net:read()
	if not ret or not ret:find("^250") then       net:error(ret);       return end
    end
    --выходим
    net:write("QUIT\n")

    if argv.generror then net:error() return end
    net:done()
end
