server:server.cc
	g++ -o $@ $^ -std=c++17 -lfmt -lpthread -lstdc++fs -lsnappy -ljsoncpp
.PHONEY:clean
clean:
	rm -rf server cloud.dat backdir packdir