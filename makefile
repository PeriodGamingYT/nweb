make: main.c
	gcc -o nweb main.c

clean:
	rm -f nweb

kill:
	killall nweb

run:
	make clean
	make
	clear
	./nweb .

install:
	sudo cp nweb /usr/local/bin
