#!/usr/bin/lua


module={
    type="Tcp",
    name="test"
}

function main() 
    print("1. запуск — main")

    print("2. запуск — net=api:connect()")
    net=api:connect()
    print("tostring: "..tostring(net))
    print("3. запуск — net:write()")
    net:write("write\000044")

    print("4. запуск — net:read()")
    print("	прочитано — "..net:read())


    print("5. запуск — net:done()")
    net:done()
    print("6. эта строка никогда не должна появиться")
end

