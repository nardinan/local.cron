objects = jobs.o local.cron.o
cc = gcc -g
cflags = -Wall -I.. -I/usr/local/include -Wno-variadic-macros -Wno-missing-braces -Wno-pointer-sign -c
lflags = -Wall
liblink = -L/usr/local/lib -lpthread -lmiranda_ground
exec = local.cron.bin

all: $(objects)
	$(cc) $(lflags) $(objects) -o $(exec) $(liblink)

jobs.o: jobs.c jobs.h
	$(cc) $(cflags) jobs.c

local.cron.o: local.cron.c jobs.h
	$(cc) $(cflags) local.cron.c

clean:
	rm -f *.o
	rm -f $(exec)
