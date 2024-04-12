# Nonfile Targets
all: ft

clean: 
	rm -f ft ft.o ft_client.o nodeFT.o dynarray.o path.o

clobber: clean 
	rm -f meminfo*.out

# File Target
ft: ft.o ft_client.o nodeFT.o dynarray.o path.o
	gcc217 -g ft.o ft_client.o nodeFT.o dynarray.o path.o -o ft

ft.o: ft.c dynarray.h path.h a4def.h ft.h nodeFT.h
	gcc217 -g -c ft.c

ft_client.o: ft_client.c ft.h a4def.h
	gcc217 -g -c ft_client.c

nodeFT.o: nodeFT.c nodeFT.h dynarray.h a4def.h path.h
	gcc217 -g -c nodeFT.c

dynarray.o: dynarray.c dynarray.h
	gcc217 -g -c dynarray.c

path.o: path.c path.h a4def.h dynarray.h
	gcc217 -g -c path.c