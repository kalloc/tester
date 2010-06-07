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
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_CONF=Release
CND_DISTDIR=dist

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=build/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/report.o \
	${OBJECTDIR}/test_lua.o \
	${OBJECTDIR}/yxml.o \
	${OBJECTDIR}/test_lookup.o \
	${OBJECTDIR}/verifer.o \
	${OBJECTDIR}/aes/aes.o \
	${OBJECTDIR}/tester.o \
	${OBJECTDIR}/resolv.o \
	${OBJECTDIR}/test_icmp.o \
	${OBJECTDIR}/tools.o \
	${OBJECTDIR}/client.o \
	${OBJECTDIR}/test_tcp.o \
	${OBJECTDIR}/process.o \
	${OBJECTDIR}/statistics.o \
	${OBJECTDIR}/curve25519-donna/curve25519-donna.o

# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	${MAKE}  -f nbproject/Makefile-Release.mk dist/Release/GNU-Linux-x86/tester

dist/Release/GNU-Linux-x86/tester: ${OBJECTFILES}
	${MKDIR} -p dist/Release/GNU-Linux-x86
	${LINK.c} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/tester ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/report.o: nbproject/Makefile-${CND_CONF}.mk report.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/report.o report.c

${OBJECTDIR}/test_lua.o: nbproject/Makefile-${CND_CONF}.mk test_lua.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/test_lua.o test_lua.c

${OBJECTDIR}/yxml.o: nbproject/Makefile-${CND_CONF}.mk yxml.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/yxml.o yxml.c

${OBJECTDIR}/test_lookup.o: nbproject/Makefile-${CND_CONF}.mk test_lookup.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/test_lookup.o test_lookup.c

${OBJECTDIR}/verifer.o: nbproject/Makefile-${CND_CONF}.mk verifer.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/verifer.o verifer.c

${OBJECTDIR}/aes/aes.o: nbproject/Makefile-${CND_CONF}.mk aes/aes.c 
	${MKDIR} -p ${OBJECTDIR}/aes
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/aes/aes.o aes/aes.c

${OBJECTDIR}/tester.o: nbproject/Makefile-${CND_CONF}.mk tester.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/tester.o tester.c

${OBJECTDIR}/resolv.o: nbproject/Makefile-${CND_CONF}.mk resolv.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/resolv.o resolv.c

${OBJECTDIR}/test_icmp.o: nbproject/Makefile-${CND_CONF}.mk test_icmp.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/test_icmp.o test_icmp.c

${OBJECTDIR}/tools.o: nbproject/Makefile-${CND_CONF}.mk tools.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/tools.o tools.c

${OBJECTDIR}/client.o: nbproject/Makefile-${CND_CONF}.mk client.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/client.o client.c

${OBJECTDIR}/test_tcp.o: nbproject/Makefile-${CND_CONF}.mk test_tcp.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/test_tcp.o test_tcp.c

${OBJECTDIR}/process.o: nbproject/Makefile-${CND_CONF}.mk process.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/process.o process.c

${OBJECTDIR}/statistics.o: nbproject/Makefile-${CND_CONF}.mk statistics.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/statistics.o statistics.c

${OBJECTDIR}/curve25519-donna/curve25519-donna.o: nbproject/Makefile-${CND_CONF}.mk curve25519-donna/curve25519-donna.c 
	${MKDIR} -p ${OBJECTDIR}/curve25519-donna
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/curve25519-donna/curve25519-donna.o curve25519-donna/curve25519-donna.c

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r build/Release
	${RM} dist/Release/GNU-Linux-x86/tester

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
