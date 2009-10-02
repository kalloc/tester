#!/usr/bin/lua
require "lib"

local host="n1ck.name"
local id="66666666"


local AllTest={
[    http={
	default={host="seostatus.ru",port=80,id=id},
	task={
    	    "1313get1010",
--	    "1313get18tesz.php16test=1",
--	    "1314post18tesz.php16test=1",
--	    "1317chksize18test.php147001",
--	    "1418chkwords1011116zzzz",
--	    "1414auth18tesz.php15admin18password"
	    
	}
    },
    ftp={
	default={host=host,port=21,id=id},
	task={
--		"1314auth210zolotester1812341234",
		"1414list210solotester181234123413/dd",
--                "1517chkfile210solotester181234123411/14filz",
--		"1413cwd210solotester181234123413/dd",
		"1514retr210solotester181234123411/14filz",
		"1513put210solotester181234123411/18rootperm",
		"1516delete210solotester181234123411/18rootperm"
	}
    },
--[[
    smtp={
	default={host=host,port=25,id=id},
	task={
		"1314auth220zolotester@n1ck.name1812341234",
		"1314send220zolotester@n1ck.name1812341234"
	}
    },
]]
--[[
    pop3={
	default={host=host,port=110,id=id},
	task={
		"1314auth220zolotester@n1ck.name1812341234",
		"1314list220zolotester@n1ck.name1812341234",
		"1514retr220solotester@n1ck.name1812341234220solotester@n1ck.name17Testerz",
		"1314noop220solotester@n1ck.name1812341234",
		"1516delete220solotester@n1ck.name1812341234220solotester@n1ck.name17Testerz",
		"1718chkwords220solotester@n1ck.name1812341234220solotester@n1ck.name16Tester11117Serverz"
	}
    }
    ]]
--[[
    ,dns={
	default={host=host,port=53,id=id},
	task={
	    "1315cname1313210",
	    "1315cname1019znachenie",
	    "1313rtr1343219znachenie"
	}
    }
    ]]
}
--print(run("module_http.lua",))
for test,testtable in pairs(AllTest) do
    print(string.rep("-",100))
    print("|             самотестирование модуля - "..test)
    print(string.rep("-",100).."\n")
	    
    for key,task in pairs(testtable.task) do
        print(string.rep("-",100))
        print("|             конфиг - "..task)
	print(string.rep("-",100))
	local argv=clone(testtable.default)
	method , data = unPackData(task)
	argv.method=method
	argv.data=data
	print(run("module_"..test..".lua",argv))
    end

end
