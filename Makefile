
all:
	g++ Debug.cpp test.cpp -o test -lpthread -std=c++14

Debug.o: Debug.cpp
	g++ Debug.cpp -c -std=c++14

lib: Debug.o
	ar r Debug.a Debug.o

