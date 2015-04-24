missandout: main.c
	gcc -g -Wall $< -o$@ -lpthread

clean:
	rm missandout
