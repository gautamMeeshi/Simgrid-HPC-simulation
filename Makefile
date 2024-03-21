compile:
	g++ multinode-multicore.cpp helper.hpp scheduler.hpp -o exec -lsimgrid

run:
	./exec small_platform.xml deployment.xml