PROGRAMS	= server_shm server_msq process_producer process_consumer

# Link programs with the pthread library.
LDLIBS		+= -lpthread -lm

.PHONY:         all clean

# build programs 
all:            $(PROGRAMS) 

# Clean up build products.
clean:
	rm -f *.o *.txt* $(PROGRAMS) 
