sender: s.cpp
receiver: r.cpp

usage:
1. compile
g++ s.cpp -o s
g++ r.cpp -o r

2. run
./r
./s [ip of receiver] [file name]
example: ./s 192.168.1.1 data.bin