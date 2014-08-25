# termdraw - terminal drawing program
# Makefile taken from suckless style.

include config.mk

HEADERS = termdraw.h
SRC = termdraw.c
OBJ = ${SRC:.c=.o}

all: options termdraw

options:
	@echo determinant build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"
	@echo "LD       = ${LD}"

%.o: %.c
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

${OBJ}: config.mk

termdraw: ${OBJ}
	@echo LD $@
	@${LD} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f determinant ${OBJ} ${PROG_NAME}-${VERSION}.tar.gz

dist: clean
	@echo creating dist tarball
	@mkdir -p ${PROG_NAME}-${VERSION}
	@cp -R Makefile LICENSE README.md config.mk ${SRC} ${HEADERS} ${PROG_NAME}-${VERSION}
	@tar -cf ${PROG_NAME}-${VERSION}.tar ${PROG_NAME}-${VERSION}
	@gzip ${PROG_NAME}-${VERSION}.tar
	@rm -rf ${PROG_NAME}-${VERSION}

.PHONY: all options clean dist

