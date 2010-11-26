#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=gcc
CXX=gcc
FC=
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_CONF=Debug
CND_DISTDIR=dist

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=build/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/tester.o \
	${OBJECTDIR}/client.o \
	${OBJECTDIR}/curve25519-donna/curve25519-donna.o \
	${OBJECTDIR}/verifer.o \
	${OBJECTDIR}/test_lookup.o \
	${OBJECTDIR}/resolv.o \
	${OBJECTDIR}/report.o \
	${OBJECTDIR}/test_lua.o \
	${OBJECTDIR}/tools.o \
	${OBJECTDIR}/aes/aes.o \
	${OBJECTDIR}/statistics.o \
	${OBJECTDIR}/process.o \
	${OBJECTDIR}/test_icmp.o \
	${OBJECTDIR}/test_tcp.o


# C Compiler Flags
CFLAGS=-I/opt/tester/include/ -I/usr/include/libxml2/

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-L/opt/tester/lib/ -lz -lssl -lm /opt/tester/lib/liblua.a /opt/tester/lib/libevent.a -lpthread -lrt /opt/tester/lib/libevent_pthreads.a /opt/tester/lib/libcares.a -lxml2

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-Debug.mk dist/Debug/GNU-Linux-x86/tester

dist/Debug/GNU-Linux-x86/tester: /opt/tester/lib/liblua.a

dist/Debug/GNU-Linux-x86/tester: /opt/tester/lib/libevent.a

dist/Debug/GNU-Linux-x86/tester: /opt/tester/lib/libevent_pthreads.a

dist/Debug/GNU-Linux-x86/tester: /opt/tester/lib/libcares.a

dist/Debug/GNU-Linux-x86/tester: ${OBJECTFILES}
	${MKDIR} -p dist/Debug/GNU-Linux-x86
	${LINK.c} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/tester ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/tester.o: tester.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/opt/tester/include -I/usr/include/libxml2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/tester.o tester.c

${OBJECTDIR}/client.o: client.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/opt/tester/include -I/usr/include/libxml2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/client.o client.c

${OBJECTDIR}/curve25519-donna/curve25519-donna.o: curve25519-donna/curve25519-donna.c 
	${MKDIR} -p ${OBJECTDIR}/curve25519-donna
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/opt/tester/include -I/usr/include/libxml2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/curve25519-donna/curve25519-donna.o curve25519-donna/curve25519-donna.c

${OBJECTDIR}/verifer.o: verifer.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/opt/tester/include -I/usr/include/libxml2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/verifer.o verifer.c

${OBJECTDIR}/test_lookup.o: test_lookup.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/opt/tester/include -I/usr/include/libxml2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/test_lookup.o test_lookup.c

${OBJECTDIR}/resolv.o: resolv.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/opt/tester/include -I/usr/include/libxml2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/resolv.o resolv.c

${OBJECTDIR}/report.o: report.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/opt/tester/include -I/usr/include/libxml2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/report.o report.c

${OBJECTDIR}/test_lua.o: test_lua.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/opt/tester/include -I/usr/include/libxml2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/test_lua.o test_lua.c

${OBJECTDIR}/tools.o: tools.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/opt/tester/include -I/usr/include/libxml2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/tools.o tools.c

${OBJECTDIR}/aes/aes.o: aes/aes.c 
	${MKDIR} -p ${OBJECTDIR}/aes
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/opt/tester/include -I/usr/include/libxml2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/aes/aes.o aes/aes.c

${OBJECTDIR}/statistics.o: statistics.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/opt/tester/include -I/usr/include/libxml2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/statistics.o statistics.c

${OBJECTDIR}/process.o: process.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/opt/tester/include -I/usr/include/libxml2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/process.o process.c

${OBJECTDIR}/test_icmp.o: test_icmp.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/opt/tester/include -I/usr/include/libxml2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/test_icmp.o test_icmp.c

${OBJECTDIR}/test_tcp.o: test_tcp.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/opt/tester/include -I/usr/include/libxml2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/test_tcp.o test_tcp.c

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r build/Debug
	${RM} dist/Debug/GNU-Linux-x86/tester

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
