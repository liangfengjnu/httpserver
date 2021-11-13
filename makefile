main:httpserver.o main.cpp
	g++ -pthread -Wall -g  httpconn.o main.cpp 
httpserver.o:httpconn.cpp
	g++ -pthread -Wall -g httpconn.cpp -c
clean:
	-rm -rf %.o
