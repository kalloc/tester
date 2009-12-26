#!/usr/bin/python
import sys,os,xmpp,time,platform

if len(sys.argv) < 2:  
    print "Usage: "+sys.argv[0]+" JID text"
    sys.exit(0)

jid=xmpp.protocol.JID("notify-me@ainmarh.com/"+platform.node())
cl=xmpp.Client(jid.getDomain(),debug=[])
con=cl.connect()
if not con: sys.exit()
auth=cl.auth(jid.getNode(),"notifyPaSsWoRd",resource=jid.getResource())
if not auth: sys.exit()
id=cl.send(xmpp.protocol.Message(sys.argv[1],' '.join(sys.argv[2:])))
time.sleep(1)   
cl.disconnect()
