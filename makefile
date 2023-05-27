all:	Manager talker

Manager:	manager.o
	gcc	-o Manager	manager.o

talker:	talker.o
	gcc	-o talker	talker.o

manager.o:	manager.c
	gcc	-c	manager.c

talker.o:	talker.c
	gcc	-c	talker.c