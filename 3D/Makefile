#
# Makefile - simulator suite make file
#
# This file is a part of the SimpleScalar tool suite written by
# Todd M. Austin as a part of the Multiscalar Research Project.
#  
# The tool suite is currently maintained by Doug Burger and Todd M. Austin.
# 
# Copyright (C) 1994, 1995, 1996, 1997, 1998 by Todd M. Austin
#
# This source file is distributed "as is" in the hope that it will be
# useful.  It is distributed with no warranty, and no author or
# distributor accepts any responsibility for the consequences of its
# use. 
#
# Everyone is granted permission to copy, modify and redistribute
# this source file under the following conditions:
#
#    This tool set is distributed for non-commercial use only. 
#    Please contact the maintainer for restrictions applying to 
#    commercial use of these tools.
#
#    Permission is granted to anyone to make or distribute copies
#    of this source code, either as received or modified, in any
#    medium, provided that all copyright notices, permission and
#    nonwarranty notices are preserved, and that the distributor
#    grants the recipient permission for further redistribution as
#    permitted by this document.
#
#    Permission is granted to distribute this file in compiled
#    or executable form under the same conditions that apply for
#    source code, provided that either:
#
#    A. it is accompanied by the corresponding machine-readable
#       source code,
#    B. it is accompanied by a written offer, with no time limit,
#       to give anyone a machine-readable copy of the corresponding
#       source code in return for reimbursement of the cost of
#       distribution.  This written offer must permit verbatim
#       duplication by anyone, or
#    C. it is distributed by someone who received only the
#       executable form, and is accompanied by a copy of the
#       written offer of source code that they received concurrently.
#
# In other words, you are welcome to use, share and improve this
# source file.  You are forbidden to forbid anyone else to use, share
# and improve what you give them.
#
# INTERNET: dburger@cs.wisc.edu
# US Mail:  1210 W. Dayton Street, Madison, WI 53706
#
# $Id: Makefile,v 1.1.1.1 2011/03/29 01:04:34 huq Exp $
#
# $Log: Makefile,v $
# Revision 1.1.1.1  2011/03/29 01:04:34  huq
# setup project CMP
#
# Revision 1.2  2010/04/30 19:05:57  xue
# *** empty log message ***
#
# Revision 1.1.1.1  2010/02/18 21:22:12  xue
# CMP_NETWORK
#
# Revision 1.6  2009/07/09 20:36:32  garg
# Changes related to write buffer.
#
# Revision 1.5  2009/06/01 14:51:11  xue
# Off chip memory modeling + split notification + multiple virtual channels for popnet
#
# Revision 1.4  2009/05/28 16:03:47  garg
# Makefile upgraded to make both popnet and FSOI networks from the top.
#
# Revision 1.3  2009/03/05 16:58:23  garg
# Updated version of the CMP network simulator.
#
# Revision 1.9  2005/03/11 01:36:56  rashid
# *** empty log message ***
#
# Revision 1.8  2005/01/31 23:48:35  rashid
# *** empty log message ***
#
# Revision 1.7  2004/11/29 22:46:07  rashid
# *** empty log message ***
#
# Revision 1.6  2004/11/17 05:32:39  rashid
# *** empty log message ***
#
# Revision 1.5  2004/11/16 16:16:55  rashid
# *** empty log message ***
#
# Revision 1.4  2004/11/14 21:38:26  rashid
# *** empty log message ***
#
# Revision 1.3  2004/11/12 19:03:19  rashid
# *** empty log message ***
#
# Revision 1.2  2004/11/02 22:20:28  rashid
# *** empty log message ***
#
# Revision 1.1  2004/10/01 02:44:09  rashid
# Initial revision
#
# Revision 1.23  2004/08/16 14:47:37  grbriggs
# Add #-Wfloat-equal
#
# Revision 1.22  2004/08/03 01:34:55  ninelson
# greg made changes, let him own it
#
# Revision 1.21  2004/07/28 04:55:02  grbriggs
# Incomplete adding of tokencoherence
#
# Revision 1.20  2004/07/20 16:34:18  grbriggs
# Add some -DNDEBUG's where appropriate.
#
# Revision 1.19  2004/07/16 19:27:11  grbriggs
# Add boundschecking target, to build-in an overflow detector.
#
# Revision 1.18  2004/07/14 18:07:29  grbriggs
# Add -march to target 'profiling'
#
# Revision 1.17  2004/07/13 22:07:52  grbriggs
# Add header.h dependancies.
#
# Revision 1.16  2004/07/13 00:31:41  ninelson
# removed -N_WAY from command line option
#
# Revision 1.15  2004/07/13 00:16:57  ninelson
# Default N_WAY is now 2
#
# Revision 1.14  2004/07/07 14:43:28  grbriggs
# Add missing dependancy.
#
# Revision 1.13  2004/07/01 17:39:27  grbriggs
# Turn on -static for -O0 and improve nway_all optimization flags.
#
# Revision 1.12  2004/06/29 16:41:19  ninelson
# *** empty log message ***
#
# Revision 1.11  2004/06/21 21:02:16  grbriggs
# Add gcc 3.x/pentium 4 optimization mode.
#
# Revision 1.10  2004/06/21 20:10:24  grbriggs
# Better optimization; support for making *.s
#
# Revision 1.9  2004/06/21 15:55:59  grbriggs
# Allowed any .i file to be made via a general rule.
#
# Revision 1.8  2004/06/21 15:19:53  ninelson
# made changes.
#
# Revision 1.7  2004/06/14 18:10:46  grbriggs
# Fix power.o's dependancies
#
# Revision 1.6  2004/06/10 19:45:45  grbriggs
# Better compiler optimizations.
#
# Revision 1.5  2004/06/09 17:36:10  grbriggs
# Added build target "profiling". Also, cause make opt and make profiling to turn off -DDEBUG.
#
# Revision 1.4  2004/06/09 17:30:37  ninelson
# added N_WAY flag and reversed order of compilation
#
# Revision 1.3  2004/05/25 01:10:34  ninelson
# just changes
#
# Revision 1.2  2003/10/20 14:02:53  ninelson
# *** empty log message ***
#
# Revision 1.8  1998/08/31 17:05:49  taustin
# added support for MS VC++ NMAKE builds (works with v5.0 SR3 or later)
#
# Revision 1.7  1998/08/27 07:40:03  taustin
# reorganized Makefile: it now works with MS VC++ NMAKE, and many host
#    configurations are supplied in the header; added target configuration
#    support; converted "sim-tests" target to use "-redir:sim" and
#    "-redir:prog" options, this eliminates the need for the "redir"
#    scripts
#
# Revision 1.6  1997/04/16  22:08:40  taustin
# added standalone loader support
#
# Revision 1.5  1997/03/11  01:04:13  taustin
# updated copyright
# CC target now supported
# RANLIB target now supported
# MAKE target now supported
# CFLAGS reorganized
# MFLAGS and MLIBS to improve portability
#
# Revision 1.1  1996/12/05  18:56:09  taustin
# Initial revision
#
# 

##################################################################
#
# Modify the following definitions to suit your build environment,
# NOTE: most platforms should not require any changes
#
##################################################################

#
# Define below C compiler and flags, machine-specific flags and libraries,
# build tools and file extensions, these are specific to a host environment,
# pre-tested environments follow...
#

##
## vanilla Unix, GCC build
##
## NOTE: the SimpleScalar simulators must be compiled with an ANSI C
## compatible compiler.
##
## tested hosts:
##
##	Slackware Linux version 2.0.33, GNU GCC version 2.7.2.2
##	FreeBSD version 3.0-current, GNU egcs version 2.91.50
##	Alpha OSF1 version 4.0, GNU GCC version 2.7.2
##	PA-RISC HPUX version B.10.01, GNU GCC version 2.7-96q3
##	SPARC SunOS version 5.5.1, GNU egcs-2.90.29
##	RS/6000 AIX Unix version 4, GNU GCC version cygnus-2.7-96q4
##	Windows NT version 4.0, Cygnus CygWin/32 beta 19
##
CCPP = g++
CC = gcc
OFLAGS = -O0 -g -w -Wall   -m32 -Wno-unused
#OFLAGS = -O3 -g3 -Wall      
#OFLAGS = -O0 -g -static
MFLAGS = `./sysprobe -flags`
MLIBS  = `./sysprobe -libs` -lm
ENDIAN = `./sysprobe -s`
MAKE = make
AR = ar qcv
AROPT =
RANLIB = ranlib
RM = rm -f
RMDIR = rm -f
LN = ln -s
LNDIR = ln -s
DIFF = diff
OEXT = o
LEXT = a
EEXT =
CS = ;
X=/

##
## Alpha OSF1 version 4.0, DEC C compiler version V5.2-036
##
#CC = cc -std
#OFLAGS = -O0 -g -w
#MFLAGS = `./sysprobe -flags`
#MLIBS  = `./sysprobe -libs` -lm
#ENDIAN = `./sysprobe -s`
#MAKE = make
#AR = ar qcv
#AROPT =
#RANLIB = ranlib
#RM = rm -f
#RMDIR = rm -f
#LN = ln -s
#LNDIR = ln -s
#DIFF = diff
#OEXT = o
#LEXT = a
#EEXT =
#CS = ;
#X=/

##
## PA-RISC HPUX version B.10.01, c89 HP C compiler version A.10.31.02
##
#CC = c89 +e -D__CC_C89
#OFLAGS = -g
#MFLAGS = `./sysprobe -flags`
#MLIBS  = `./sysprobe -libs` -lm
#ENDIAN = `./sysprobe -s`
#MAKE = make
#AR = ar qcv
#AROPT =
#RANLIB = ranlib
#RM = rm -f
#RMDIR = rm -f
#LN = ln -s
#LNDIR = ln -s
#DIFF = diff
#OEXT = o
#LEXT = a
#EEXT =
#CS = ;
#X=/

##
## SPARC SunOS version 5.5.1, Sun WorkShop C Compiler (acc) version 4.2
##
#CC = /opt/SUNWspro/SC4.2/bin/acc
#OFLAGS = -O0 -g
#MFLAGS = `./sysprobe -flags`
#MLIBS  = `./sysprobe -libs` -lm
#ENDIAN = `./sysprobe -s`
#MAKE = make
#AR = ar qcv
#AROPT =
#RANLIB = ranlib
#RM = rm -f
#RMDIR = rm -f
#LN = ln -s
#LNDIR = ln -s
#DIFF = diff
#OEXT = o
#LEXT = a
#EEXT =
#CS = ;
#X=/

##
## RS/6000 AIX Unix version 4, xlc compiler build
##
#CC = xlc -D__CC_XLC
#OFLAGS = -g
#MFLAGS = `./sysprobe -flags`
#MLIBS  = `./sysprobe -libs` -lm
#ENDIAN = `./sysprobe -s`
#MAKE = make
#AR = ar qcv
#AROPT =
#RANLIB = ranlib
#RM = rm -f
#RMDIR = rm -f
#LN = ln -s
#LNDIR = ln -s
#DIFF = diff
#OEXT = o
#LEXT = a
#EEXT =
#CS = ;
#X=/

##
## WinNT, MS VC++ build
##
## NOTE: requires MS VC++ version 5.0 + service pack 3 or later
## NOTE1: before configuring the simulator, delete the symbolic link "tests/"
##
#CC = cl /Za /nologo
#OFLAGS = /W3 /Zi
#MFLAGS = -DBYTES_LITTLE_ENDIAN -DWORDS_LITTLE_ENDIAN -DFAST_SRL -DFAST_SRA
#MLIBS  =
#ENDIAN = little
#MAKE = nmake /nologo
#AR = lib
#AROPT = -out:
#RANLIB = dir
#RM = del/f/q
#RMDIR = del/s/f/q
#LN = copy
#LNDIR = xcopy/s/e/i
#DIFF = dir
#OEXT = obj
#LEXT = lib
#EEXT = .exe
#CS = &&
#X=\\\\

#
# Compilation-specific feature flags
#
# -DDEBUG	- turns on debugging features
# -DBFD_LOADER	- use libbfd.a to load programs (also required BINUTILS_INC
#		  and BINUTILS_LIB to be defined, see below)
# -DGZIP_PATH	- specifies path to GZIP executable, only needed if SYSPROBE
#		  cannot locate binary
# -DSLOW_SHIFTS	- emulate all shift operations, only used for testing as
#		  sysprobe will auto-detect if host can use fast shifts
#
FLAGS = -DDEBUG  -DMTA -DDEBUG1 -w -DREMOVE

#FFLAGS = -DDEBUG
#NWAY = 2
#NWAYFLAGS = -DN_WAY=$(NWAY)

#ALL_NWAY = 2 4 8

#
# Point the Makefile to your Simplescalar-based bunutils, these definitions
# should indicate where the include and library directories reside.
# NOTE: these definitions are only required if BFD_LOADER is defined.
#
#BINUTILS_INC = -I../include
#BINUTILS_LIB = -L../lib

#
#


##################################################################
#
# YOU SHOULD NOT NEED TO MODIFY ANYTHING BELOW THIS COMMENT
#
##################################################################

#
# complete flags
#
#CFLAGS = $(MFLAGS) $(FFLAGS) $(OFLAGS) $(BINUTILS_INC) $(BINUTILS_LIB)
#$(NWAYFLAGS)
#SAFECFLAGS = $(MFLAGS) $(FFLAGS) $(SAFEOFLAGS) $(BINUTILS_INC) $(BINUTILS_LIB) 

CFLAGS = $(MFLAGS) $(FFLAGS) $(OFLAGS) $(BINUTILS_INC) $(BINUTILS_LIB) $(FLAGS)
CPPFLAGS = -g -m32
SAFECFLAGS = $(MFLAGS) $(FFLAGS) $(SAFEOFLAGS) $(BINUTILS_INC) $(BINUTILS_LIB) 

#
# all the sources
#
#

MAINSIMSRC = sim-outorder$(MM)
BPREDSRC = bpred$(MM)

HOTSRCS = temperature.c RCutil.c flp.c util.c hotspot.c
HOTHDRS	= RC.h flp.h util.h hotspot.h
HOTOBJS = temperature.o RCutil.o flp.o util.o hotspot.o

SRCS =	main.c sim-fast.c sim-safe.c sim-cache.c sim-profile.c \
	sim-eio.c sim-bpred.c sim-cheetah.c $(MAINSIMSRC).c  mystats.c\
	memory.c regs.c cache.c $(BPREDSRC).c ptrace.c eventq.c network.c\
	resource.c endian.c dlite.c symbol.c eval.c options.c range.c \
	eio.c stats.c endian.c misc.c power.c context.c cluster.c cmt.c\
	interconnect.c $(HOTSRCS) tokencoherence.c branchCorr.c instManip.c \



HDRS =	syscall.h memory.h regs.h sim.h loader.h cache.h bpred.h ptrace.h network.h\
	eventq.h resource.h endian.h dlite.h symbol.h eval.h bitmap.h mystats.h\
	eio.h range.h version.h endian.h misc.h power.h context.h smt.h  cluster.h MTA.h\
	headers.h interconnect.h $(HOTHDRS) tokencoherence.h branchCorr.h instManip.h \

SRCSPP = skeleton_chk.cpp store2Load.cpp indirectjump.cpp

#
# common objects
#
OBJS =	main.$(OEXT) syscall.$(OEXT) memory.$(OEXT) regs.$(OEXT) \
	loader.$(OEXT) endian.$(OEXT) dlite.$(OEXT) symbol.$(OEXT) \
	eval.$(OEXT) options.$(OEXT) stats.$(OEXT) eio.$(OEXT) \
	range.$(OEXT) misc.$(OEXT) power.$(OEXT) context.$(OEXT) cluster.$(OEXT) cmt.$(OEXT) \
	machine.$(OEXT) \
	cluster-opts.$(OEXT) interconnect.$(OEXT) network.$(OEXT) mystats.$(OEXT)\
	$(HOTOBJS) tokencoherence.$(OEXT) branchCorr.$(OEXT) instManip.$(OEXT) \
 	skeleton_chk.$(OEXT) store2Load.$(OEXT) indirectjump.$(OEXT)

#
# programs to build
#
PROGS = sim-fast$(EEXT) sim-safe$(EEXT) sim-eio$(EEXT) \
	sim-bpred$(EEXT) sim-profile$(EEXT) \
	sim-cheetah$(EEXT) sim-cache$(EEXT) $(MAINSIMSRC)$(EEXT)

SIMOUTORDERDEP1 = sysprobe$(EEXT) $(MAINSIMSRC).$(OEXT) 
SIMOUTORDERDEP2 = sysprobe$(EEXT) cache.$(OEXT) $(BPREDSRC).$(OEXT) network.$(OEXT) mystats.$(OEXT) resource.$(OEXT) ptrace.$(OEXT) $(OBJS)
SIMOUTORDERDEP = $(SIMOUTORDERDEP1) $(SIMOUTORDERDEP2)

#
# network to buil : popnet relay network or free-space optical network
#
POPNET = popnet
POPNETLIB = ./popnetForSimplescalar/*.o ./popnetForSimplescalar/orion_power_beta_mar2003/power/*.o

FSOI = fsoi
FSOILIB = ./opticalForSimplescalar/*.o


#
# all targets, NOTE: library ordering is important...
#
all: oldall
	@echo "my work is done here..."

efencegcc2:
	make sysprobe
	make -j 2 sim-outorder MLIBS='$(MLIBS) -lefence'

efencegcc3:
	make sysprobe
	make -j 2 sim-outorder MLIBS='$(MLIBS) -L/usr/eearch1/data/nick/ss/cluster -lpthread -lefence'

dynamic:
	make sysprobe
	make -j 2 sim-outorder OFLAGS=' -g3 -gdwarf-2 -ggdb3  -g -Wall -march=i686'

opt:
	make clean
	make sysprobe
	make -j 2 sim-outorder OFLAGS='-march=i686 -O3 -funroll-loops -fomit-frame-pointer' FFLAGS='-DNDEBUG'

optp4:  #this only works on gcc 3.x
	make clean
	make sysprobe
	make -j 2 sim-outorder OFLAGS='-march=pentium4 -msse2 -O3 -funroll-loops -fomit-frame-pointer -mfpmath=sse -static' FFLAGS='-DNDEBUG'

profiling:
	make clean
	make sysprobe
	make -j 2 sim-outorder OFLAGS='-march=i686 -O3 -funroll-loops -pg' FFLAGS='-DNDEBUG'

boundschecking:
	make clean
	make sysprobe
	make -j 2 sim-outorder OFLAGS='-march=pentium4 -msse2 -fbounds-checking -B/usr/eearch18/data/nick/build-bcgcc/gcc/' FFLAGS='' CC='/usr/eearch18/data/nick/build-bcgcc/gcc/xgcc'

#opt_all:
#	for N in $(ALL_NWAY) ; do \
#	  make opt_nway NWAY=$$N; \
#	done

#opt_nway:
#	make clean
#	make sysprobe
#	make -j 2 sim-outorder OFLAGS='-march=i686 -O3 -funroll-loops -fomit-frame-pointer'
	cp -f ./sim-outorder /usr/eearch1/data/nick/ optical/sims/sim-cluster16-$(NWAY)

oldall2: 
	rsh -n durand make -s -C ~/Research/ss/cluster $(SIMOUTORDERDEP)&
	@make sim-outorder
	@echo "my work is done here..."

dist_sort:
	make dist_doit
	echo "about done"
	make sim-outorder

dist_doit:
	rsh -n durand make -s -C ~/Research/ss/cluster durand_doit&
	make cayuga_doit 
	echo "done"


durand_doit : $(SIMOUTORDERDEP1)
	echo "durand done"
	
cayuga_doit : $(SIMOUTORDERDEP2)
	echo "cayuga done"
	
oldall: $(MAINSIMSRC)


config-pisa:
	-$(RM) machine.h machine.c machine.def loader.c symbol.c syscall.c
	$(LN) target-pisa$(X)pisa.h machine.h
	$(LN) target-pisa$(X)pisa.c machine.c
	$(LN) target-pisa$(X)pisa.def machine.def
	$(LN) target-pisa$(X)loader.c loader.c
	$(LN) target-pisa$(X)symbol.c symbol.c
	$(LN) target-pisa$(X)syscall.c syscall.c
	-$(RMDIR) tests
	$(LNDIR) tests-pisa tests

config-alpha:
	-$(RM) machine.h machine.c machine.def loader.c symbol.c syscall.c
	$(LN) target-alpha$(X)alpha.h machine.h
	$(LN) target-alpha$(X)alpha.c machine.c
	$(LN) target-alpha$(X)alpha.def machine.def
	$(LN) target-alpha$(X)loader.c loader.c
	$(LN) target-alpha$(X)symbol.c symbol.c
	$(LN) target-alpha$(X)syscall.c syscall.c
	-$(RMDIR) tests
	$(LNDIR) tests-alpha tests


config-risc:
	-$(RM) config.h machine.h machine.c machine.def loader.c symbol.c syscall.c
	$(LN) target-risc$(X)config.h config.h
	$(LN) target-risc$(X)risc.h machine.h
	$(LN) target-risc$(X)risc.c machine.c
	$(LN) target-risc$(X)risc.def machine.def
	$(LN) target-risc$(X)loader.c loader.c
	$(LN) target-risc$(X)symbol.c symbol.c
	$(LN) target-risc$(X)syscall.c syscall.c
	-$(RMDIR) tests
	$(LNDIR) tests-risc tests


sysprobe$(EEXT):	sysprobe.c
	$(CC) $(FFLAGS) -o sysprobe$(EEXT) sysprobe.c
	@echo endian probe results: $(ENDIAN)
	@echo probe flags: $(MFLAGS)
	@echo probe libs: $(MLIBS)

wattch-interact$(EEXT): sysprobe$(EEXT) eval.o misc.o stats.o power.o wattch-interact.o cacti/libcacti.$(LEXT)
	$(CC) eval.o misc.o stats.o power.o wattch-interact.o cacti/libcacti.$(LEXT) $(CFLAGS)  -lm -o wattch-interact

power$(EEXT):           power.c
	$(CC) $(SAFECFLAGS) -c power.c

sim-fast$(EEXT):	sysprobe$(EEXT) sim-fast.$(OEXT) $(OBJS) libexo/libexo.$(LEXT)
	$(CC) -o sim-fast$(EEXT) $(CFLAGS) sim-fast.$(OEXT) $(OBJS) libexo/libexo.$(LEXT) $(MLIBS)

sim-safe$(EEXT):	sysprobe$(EEXT) sim-safe.$(OEXT) $(OBJS) libexo/libexo.$(LEXT)
	$(CC) -o sim-safe$(EEXT) $(CFLAGS) sim-safe.$(OEXT) $(OBJS) libexo/libexo.$(LEXT) $(MLIBS)

sim-profile$(EEXT):	sysprobe$(EEXT) sim-profile.$(OEXT) $(OBJS) libexo/libexo.$(LEXT)
	$(CC) -o sim-profile$(EEXT) $(CFLAGS) sim-profile.$(OEXT) $(OBJS) libexo/libexo.$(LEXT) $(MLIBS)

sim-eio$(EEXT):	sysprobe$(EEXT) sim-eio.$(OEXT) $(OBJS) libexo/libexo.$(LEXT)
	$(CC) -o sim-eio$(EEXT) $(CFLAGS) sim-eio.$(OEXT) $(OBJS) libexo/libexo.$(LEXT) $(MLIBS)

sim-bpred$(EEXT):	sysprobe$(EEXT) sim-bpred.$(OEXT) bpred.$(OEXT) $(OBJS) libexo/libexo.$(LEXT)
	$(CC) -o sim-bpred$(EEXT) $(CFLAGS) sim-bpred.$(OEXT) bpred.$(OEXT) $(OBJS) libexo/libexo.$(LEXT) $(MLIBS)

sim-cheetah$(EEXT):	sysprobe$(EEXT) sim-cheetah.$(OEXT) $(OBJS) libcheetah/libcheetah.$(LEXT) libexo/libexo.$(LEXT)
	$(CC) -o sim-cheetah$(EEXT) $(CFLAGS) sim-cheetah.$(OEXT) $(OBJS) libcheetah/libcheetah.$(LEXT) libexo/libexo.$(LEXT) $(MLIBS)

sim-cache$(EEXT):	sysprobe$(EEXT) sim-cache.$(OEXT) cache.$(OEXT) $(OBJS) libexo/libexo.$(LEXT)
	$(CC) -o sim-cache$(EEXT) $(CFLAGS) sim-cache.$(OEXT) cache.$(OEXT) $(OBJS) libexo/libexo.$(LEXT) $(MLIBS)

$(MAINSIMSRC)$(EEXT): $(POPNET) $(FSOI) $(SIMOUTORDERDEP) libexo/libexo.$(LEXT) cacti/libcacti.$(LEXT) 
	$(CCPP) -o $(MAINSIMSRC)$(EEXT) $(CFLAGS) $(MAINSIMSRC).$(OEXT) cache.$(OEXT) $(BPREDSRC).$(OEXT) resource.$(OEXT) ptrace.$(OEXT) $(OBJS) libexo/libexo.$(LEXT) cacti/libcacti.$(LEXT) $(POPNETLIB) $(FSOILIB) $(MLIBS)

$(POPNET):
	cd ./popnetForSimplescalar; $(MAKE)

$(FSOI):
	cd ./opticalForSimplescalar; $(MAKE)

exo libexo/libexo.$(LEXT): sysprobe$(EEXT)
	cd libexo $(CS) \
	$(MAKE) "MAKE=$(MAKE)" "CC=$(CC)" "AR=$(AR)" "AROPT=$(AROPT)" "RANLIB=$(RANLIB)" "CFLAGS=$(MFLAGS) $(FFLAGS) $(OFLAGS)" "OEXT=$(OEXT)" "LEXT=$(LEXT)" "EEXT=$(EEXT)" "X=$(X)" "RM=$(RM)" libexo.$(LEXT)

cacti cacti/libcacti.$(LEXT): sysprobe$(EEXT)
	@cd cacti $(CS) \
	$(MAKE) "MAKE=$(MAKE)" "CC=$(CC)" "AR=$(AR)" "AROPT=$(AROPT)" "RANLIB=$(RANLIB)" "CFLAGS=$(MFLAGS) $(FFLAGS) $(SAFEOFLAGS)" "OEXT=$(OEXT)" "LEXT=$(LEXT)" "EEXT=$(EEXT)" "X=$(X)" "RM=$(RM)" libcacti.$(LEXT)

cheetah libcheetah/libcheetah.$(LEXT): sysprobe$(EEXT)
	cd libcheetah $(CS) \
	$(MAKE) "MAKE=$(MAKE)" "CC=$(CC)" "AR=$(AR)" "AROPT=$(AROPT)" "RANLIB=$(RANLIB)" "CFLAGS=$(MFLAGS) $(FFLAGS) $(OFLAGS)" "OEXT=$(OEXT)" "LEXT=$(LEXT)" "EEXT=$(EEXT)" "X=$(X)" "RM=$(RM)" libcheetah.$(LEXT)

# *.c -> *.s  Makes any .s file. Paranoid but more correct: depends on *.h,machine.def
%.s: %.c $(wildcard *.h) machine.def
	gcc -S $< -o $@

# *.c -> *.opt.s 
%.opt.s: %.c $(wildcard *.h) machine.def
	gcc -S -march=i686 -O3 -funroll-loops -fomit-frame-pointer $< -o $@

# *.c -> *.i  Makes any .i file. Paranoid but more correct: depends on *.h,machine.def
%.i: %.c $(wildcard *.h) machine.def
	gcc -E $< > $@


$(MAINSIMSRC).o: $(MAINSIMSRC).c
	$(CC) $(CFLAGS) -Wno-unused-value -c $(MAINSIMSRC).c


.c.$(OEXT):
	$(CC) $(CFLAGS) -c $*.c

filelist:
	@echo $(SRCS) $(HDRS) Makefile

diffs:
	-rcsdiff RCS/*

sim-tests sim-tests-nt: sysprobe$(EEXT) $(PROGS)
	cd tests $(CS) \
	$(MAKE) "MAKE=$(MAKE)" "RM=$(RM)" "ENDIAN=$(ENDIAN)" tests \
		"DIFF=$(DIFF)" "SIM_DIR=.." "SIM_BIN=sim-fast$(EEXT)" \
		"X=$(X)" "CS=$(CS)" $(CS) \
	cd ..
	cd tests $(CS) \
	$(MAKE) "MAKE=$(MAKE)" "RM=$(RM)" "ENDIAN=$(ENDIAN)" tests \
		"DIFF=$(DIFF)" "SIM_DIR=.." "SIM_BIN=sim-safe$(EEXT)" \
		"X=$(X)" "CS=$(CS)" $(CS) \
	cd ..
	cd tests $(CS) \
	$(MAKE) "MAKE=$(MAKE)" "RM=$(RM)" "ENDIAN=$(ENDIAN)" tests \
		"DIFF=$(DIFF)" "SIM_DIR=.." "SIM_BIN=sim-cache$(EEXT)" \
		"X=$(X)" "CS=$(CS)" $(CS) \
	cd ..
	cd tests $(CS) \
	$(MAKE) "MAKE=$(MAKE)" "RM=$(RM)" "ENDIAN=$(ENDIAN)" tests \
		"DIFF=$(DIFF)" "SIM_DIR=.." "SIM_BIN=sim-cheetah$(EEXT)" \
		"X=$(X)" "CS=$(CS)" $(CS) \
	cd ..
	cd tests $(CS) \
	$(MAKE) "MAKE=$(MAKE)" "RM=$(RM)" "ENDIAN=$(ENDIAN)" tests \
		"DIFF=$(DIFF)" "SIM_DIR=.." "SIM_BIN=sim-bpred$(EEXT)" \
		"X=$(X)" "CS=$(CS)" $(CS) \
	cd ..
	cd tests $(CS) \
	$(MAKE) "MAKE=$(MAKE)" "RM=$(RM)" "ENDIAN=$(ENDIAN)" tests \
		"DIFF=$(DIFF)" "SIM_DIR=.." "SIM_BIN=sim-profile$(EEXT)" \
		"X=$(X)" "CS=$(CS)" "SIM_OPTS=-all" $(CS) \
	cd ..
	cd tests $(CS) \
	$(MAKE) "MAKE=$(MAKE)" "RM=$(RM)" "ENDIAN=$(ENDIAN)" tests \
		"DIFF=$(DIFF)" "SIM_DIR=.." "SIM_BIN=sim-outorder$(EEXT)" \
		"X=$(X)" "CS=$(CS)" $(CS) \
	cd ..

clean:
	-$(RM) *.o *.obj core *~ Makefile.bak sysprobe$(EEXT) $(PROGS)  wattch-interact
#        $(PROGS) -- ali commented
	cd libcheetah $(CS) $(MAKE) "RM=$(RM)" "CS=$(CS)" clean $(CS) cd ..
	cd libexo $(CS) $(MAKE) "RM=$(RM)" "CS=$(CS)" clean $(CS) cd ..
	cd cacti $(CS) $(MAKE) "RM=$(RM)" "CS=$(CS)" clean $(CS) cd ..
	cd tests $(CS) $(MAKE) "RM=$(RM)" "CS=$(CS)" clean $(CS) cd ..
	cd ./popnetForSimplescalar; $(MAKE) clean; cd ..
	cd ./opticalForSimplescalar; $(MAKE) clean; cd ..

unpure:
	rm -f sim.pure *pure*.o sim.pure.pure_hardlink sim.pure.pure_linkinfo

depend:
	makedepend.local -n -x $(BINUTILS_INC) $(SRCS)


# DO NOT DELETE THIS LINE -- make depend depends on it.

main.$(OEXT): host.h misc.h machine.h machine.def endian.h version.h dlite.h
main.$(OEXT): regs.h memory.h options.h stats.h eval.h loader.h sim.h
sim-fast.$(OEXT): host.h misc.h machine.h machine.def regs.h memory.h
sim-fast.$(OEXT): options.h stats.h eval.h loader.h syscall.h dlite.h sim.h
sim-safe.$(OEXT): host.h misc.h machine.h machine.def regs.h memory.h
sim-safe.$(OEXT): options.h stats.h eval.h loader.h syscall.h dlite.h sim.h
sim-cache.$(OEXT): host.h misc.h machine.h machine.def regs.h memory.h
sim-cache.$(OEXT): options.h stats.h eval.h cache.h loader.h syscall.h
sim-cache.$(OEXT): dlite.h sim.h
sim-profile.$(OEXT): host.h misc.h machine.h machine.def regs.h memory.h
sim-profile.$(OEXT): options.h stats.h eval.h loader.h syscall.h dlite.h
sim-profile.$(OEXT): symbol.h sim.h
sim-eio.$(OEXT): host.h misc.h machine.h machine.def regs.h memory.h
sim-eio.$(OEXT): options.h stats.h eval.h loader.h syscall.h dlite.h eio.h
sim-eio.$(OEXT): range.h sim.h
sim-bpred.$(OEXT): host.h misc.h machine.h machine.def regs.h memory.h
sim-bpred.$(OEXT): options.h stats.h eval.h loader.h syscall.h dlite.h
sim-bpred.$(OEXT): bpred.h sim.h
sim-cheetah.$(OEXT): host.h misc.h machine.h machine.def regs.h memory.h
sim-cheetah.$(OEXT): options.h stats.h eval.h loader.h syscall.h dlite.h
sim-cheetah.$(OEXT): libcheetah/libcheetah.h sim.h
sim-outorder.$(OEXT): host.h misc.h machine.h machine.def regs.h memory.h
sim-outorder.$(OEXT): options.h stats.h eval.h cache.h loader.h syscall.h
sim-outorder.$(OEXT): bpred.h resource.h bitmap.h ptrace.h range.h dlite.h
sim-outorder.$(OEXT): sim.h
sim-outorder.$(OEXT): smt.h
sim-outorder.$(OEXT): interconnect.h
sim-outorder.$(OEXT): network.h
sim-outorder.$(OEXT): mystats.h
memory.$(OEXT): host.h misc.h machine.h machine.def options.h stats.h eval.h
memory.$(OEXT): memory.h
regs.$(OEXT): host.h misc.h machine.h machine.def loader.h regs.h memory.h
regs.$(OEXT): options.h stats.h eval.h
cache.$(OEXT): host.h misc.h machine.h machine.def cache.h memory.h options.h
cache.$(OEXT): stats.h eval.h
bpred.$(OEXT): host.h misc.h machine.h machine.def bpred.h stats.h eval.h
network.$(OEXT): host.h misc.h machine.h machine.def network.h stats.h eval.h
mystats.$(OEXT): machine.h machine.def mystats.h
ptrace.$(OEXT): host.h misc.h machine.h machine.def range.h ptrace.h
eventq.$(OEXT): host.h misc.h machine.h machine.def eventq.h bitmap.h
resource.$(OEXT): host.h misc.h resource.h
endian.$(OEXT): endian.h loader.h host.h misc.h machine.h machine.def regs.h
endian.$(OEXT): memory.h options.h stats.h eval.h
dlite.$(OEXT): host.h misc.h machine.h machine.def version.h eval.h regs.h
dlite.$(OEXT): memory.h options.h stats.h sim.h symbol.h loader.h range.h
dlite.$(OEXT): dlite.h
symbol.$(OEXT): host.h misc.h loader.h machine.h
symbol.$(OEXT): machine.def regs.h memory.h options.h stats.h eval.h symbol.h
eval.$(OEXT): host.h misc.h eval.h machine.h machine.def
options.$(OEXT): host.h misc.h options.h
range.$(OEXT): host.h misc.h machine.h machine.def symbol.h loader.h regs.h
range.$(OEXT): memory.h options.h stats.h eval.h range.h
eio.$(OEXT): host.h misc.h machine.h machine.def regs.h memory.h options.h
eio.$(OEXT): stats.h eval.h loader.h libexo/libexo.h host.h misc.h machine.h
eio.$(OEXT): syscall.h sim.h endian.h eio.h
stats.$(OEXT): host.h misc.h machine.h machine.def eval.h stats.h
endian.$(OEXT): endian.h loader.h host.h misc.h machine.h machine.def regs.h
endian.$(OEXT): memory.h options.h stats.h eval.h
misc.$(OEXT): host.h misc.h machine.h machine.def
power.$(OEXT): host.h misc.h machine.h machine.def cache.h memory.h options.h
power.$(OEXT): stats.h eval.h power.h smt.h


loader.$(OEXT): host.h misc.h machine.h machine.def endian.h regs.h memory.h
loader.$(OEXT): options.h stats.h eval.h sim.h eio.h loader.h

syscall.$(OEXT): host.h misc.h machine.h machine.def regs.h memory.h
syscall.$(OEXT): options.h stats.h eval.h loader.h sim.h endian.h eio.h
syscall.$(OEXT): syscall.h
symbol.$(OEXT): host.h misc.h loader.h machine.h
symbol.$(OEXT): machine.def regs.h memory.h options.h stats.h eval.h symbol.h

loader.$(OEXT): host.h misc.h machine.h machine.def endian.h regs.h memory.h
loader.$(OEXT): options.h stats.h eval.h sim.h eio.h loader.h

syscall.$(OEXT): host.h misc.h machine.h machine.def regs.h memory.h
syscall.$(OEXT): options.h stats.h eval.h loader.h sim.h endian.h eio.h
syscall.$(OEXT): syscall.h
symbol.$(OEXT): host.h misc.h loader.h machine.h machine.def regs.h memory.h
symbol.$(OEXT): options.h stats.h eval.h symbol.h 

interconnect.$(OEXT): interconnect.h
cluster.$(OEXT): cluster.h

cluster.$(OEXT): headers.h
cluster-opts.$(OEXT): headers.h
cmt.$(OEXT): headers.h
flp.$(OEXT): headers.h
hotspot.$(OEXT): headers.h
interconnect.$(OEXT): headers.h
power.$(OEXT): headers.h

RCutil.$(OEXT): headers.h
sim-outorder.$(OEXT): headers.h
syscall.$(OEXT): headers.h
temperature.$(OEXT): headers.h
tokencoherence.$(OEXT): tokencoherence.h
sim-outorder.$(OEXT): tokencoherence.h
cache.$(OEXT): tokencoherence.h

