reliability: reliability.cpp
	g++ reliability.cpp -o reliability -I. -O3

clean:
	rm -f reliability *.o
