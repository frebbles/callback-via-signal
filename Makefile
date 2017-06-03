timertest: timertest.c
	gcc -o timertest timertest.c -lrt

clean:
	rm timertest