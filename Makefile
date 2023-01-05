CC = g++
CFLAGS = -std=c++17 -lstdc++fs -lncurses

iptv: main.cpp
	$(CC) main.cpp -o iptv $(CFLAGS)

clean:
	rm iptv
