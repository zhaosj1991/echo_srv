echo_srv: echo_srv.c
	gcc -o $@ $^

clean:
	rm -f *.o echo_srv