#!/usr/bin/lua
require "lib"

local host="n1ck.name"
local id="66666666"


local AllTest={
    http={
	disable=false,
	default={host="www.banki.ru",port=80,id=id},
	task={
--	    "1518chkwords11/111212Registration210ISO-8859-1",
--	    "1314post18test.php16test=1",
--	    "1317chksize18test.php147000",
--	    "1313get11/10",
	    "1518chkwords11/111214ипотека212WINDOWS-1251",
--	    "1518chkwords211/index.html111216Обучение10",
--	    "1414auth18test.php15admin18password"
	    
	}
    },

    ftp={
	disable=true,
	default={host="87.236.184.102",port=21,id=id},
	task={
		"1314auth1010",
--		"1414list210solotester181234123410",
--		"1414list2182kphoto@radabor.ru15qwert19/catalog/",
--                "1517chkfile210solotester181234123411/14file",
--		"1514retr210solotester18123412341014file",
--		"1514retr210solotester181234123414/zzz13334",
--		"1513put210solotester181234123411/15file2",
--		"1516delete210solotester181234123411/15file2"
	}
    },

    smtp={
	disable=true,
	default={host="smtp.yandex.ru",port=25,id=id},
	task={
--		"1314auth220solotester@n1ck.name1812341234",
--		"1314auth217test@masterday.ru14test"
		"1314auth225alexander.atlex@yandex.ru212JHdb6wbsFas8"
	}
    },
    pop3={
	disable=true,
	default={host="2kphoto.ru",port=110,id=id},
	task={
---		"1718chkwords215test+2kphoto.ru212Zkwfx=q@yb<s222ivan@solomonitoring.ru216тест monitor111224Здравствуйте",
--		"1314list220solotester@n1ck.name1812341234",
--		"1514retr220solotester@n1ck.name1812341234220solotester@n1ck.name16Tester",
--		"1314noop220solotester@n1ck.name1812341234",
--		"1516delete220solotester@n1ck.name1812341234220solotester@n1ck.name16Tester",
		"1718chkwords220solotester@n1ck.name1812341234220solotester@n1ck.name16Tester11115Server"
	}
    },
    telnet={
	disable=true,
	default={host="87.236.184.102",port=23,id=id},
	task={
--	    "1314auth111111"
--	    "1618chkwords1010testsolo19kjshdf8bh",
--	    "1413cmd18testsolo19kjshdf8bh16uptime",
--	    "1618chkwords18testsolo19kjshdf8bh16uptime11113MUD",
	    "1618chkwords18testsolo19kjshdf8bh16uptime11014dayz"
	}
    },
    dns={
	disable=true,
	default={host=host,port=53,id=id},
	task={
	    "1315cname1313210",
--	    "1315cname1019znachenie",
--	    "1313rtr1343219znachenie"
	}
    }
    
}
--print(run("module_http.lua",))
for test,param in pairs(AllTest) do
    print(string.rep("-",100))
    print("|             самотестирование модуля - "..test)
    print(string.rep("-",100).."\n")
    if not param.disable then
    for key,task in pairs(param.task) do
        print(string.rep("-",100))
        print("|             конфиг - "..task)
	print(string.rep("-",100))
	local argv=clone(param.default)
	method , data = unPackData(task)
	argv.method=method
	argv.data=data
	print(run("module_"..test..".lua",argv))
    end
    end

end
