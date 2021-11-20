CC  = tcc
src = $(wildcard src/*.c)
hdr = $(wildcard src/*.h)
arg = -s -lutil -lX11 -o
out = bin/yterm

build: $(src) $(hdr)
	$(CC) $(src) $(arg) $(out)
