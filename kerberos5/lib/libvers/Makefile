# $FreeBSD$

LIB=	vers
INTERNALLIB=
SRCS=	print_version.c roken.h
CFLAGS+=-I. -I${KRB5DIR}/lib/roken

CLEANFILES= roken.h print_version.h

MAKE_ROKEN=	make-roken

roken.h:
	${MAKE_ROKEN} > ${.TARGET}

.include <bsd.lib.mk>

.PATH: ${KRB5DIR}/lib/vers
