#
# Makefile for Oberon Net server
# 22.05.2018 : [tcat] thomas.kral@email.cz
#

net: net.o
	gcc -o net net.o -lrf24 -lstdc++

net.o: net.cpp
	g++ -c net.cpp

net.o: net.h

install:
	install -m 4755 net /usr/local/bin

uninstall:
	rm -f /usr/local/bin/net

clean:
	rm -f *.o
