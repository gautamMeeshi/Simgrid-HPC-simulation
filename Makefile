compile_debug:
	g++ multinode-multicore.cpp helper.hpp scheduler.hpp objects.hpp -o exec -lsimgrid -g

compile:
	g++ multinode-multicore.cpp helper.hpp scheduler.hpp objects.hpp -o exec -lsimgrid

run:
	./exec input/small_platform.xml input/deployment.xml

run150:
	./exec input/platform-5.5.6.2-torus.xml input/deployment-5.5.6.2-torus.xml