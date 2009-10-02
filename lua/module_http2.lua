#!/usr/bin/lua


module={
    type="Tcp",
    name="HTTP2"
}

function main(argv) 
    --for n,v in argv do print(tostring(n),tostring(v)) end
    print("1. api:connect() on LObjId is: "..argv.id)
    local net=api:connect()
    local net2=api:connect("213.248.62.7",80)
    print(net2)
    print("net is "..tostring(net)..", net2 is "..tostring(net2)..",api is "..tostring(api))
    --формируем запрос
    local Request=string.format(
	"GET %s HTTP/1.1\r\nHost: %s\r\nLObjId: %d\r\n\r\n",
	argv.uri or "/",
	argv.host,
	argv.id
    )
    --пишем в буфер
    print("2. net2:write(Request) on LObjId is: "..argv.id..',net2 is '..tostring(net2)..' and net2:write is '..tostring(net2.write))
    net2:write(Request)

    print("2. net:write(Request) on LObjId is: "..argv.id..',net is '..tostring(net)..' and net:write is '..tostring(net.write))
    net:write(Request)

    print("3. net:read() on LObjId is: "..argv.id)
    local buf =net:read()
    print("3~. readed "..#buf.." byte")
    local buf2 =net2:read()
    print("3~~. readed "..#buf2.." byte")
    net2:close()
--читаем из него и возвращаем результат работы
    if string.find(buf,argv.pattern or "200 OK") then
	print("4. net:done() on LObjId is: "..argv.id)
	net:done()
	return
    else
	print("4. net:error() on LObjId is: "..argv.id)
	net:error()
	return
    end
end