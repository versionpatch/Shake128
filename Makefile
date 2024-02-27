CC = g++
CFLAGS = -O2

shake128 : shake128.cpp 
	$(CC) $(CFLAGS) shake128.cpp -o shake128
