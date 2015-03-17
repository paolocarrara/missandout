missandout: main.c
	gcc $< -o $@ -lpthread

clean:
	rm missandout
