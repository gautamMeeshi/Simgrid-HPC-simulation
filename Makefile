compile:
	g++ multinode-multicore.cpp helper.hpp -o exec -lsimgrid

run:
	./exec small_platform.xml deployment.xml