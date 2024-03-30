compile_debug:
	g++ multinode-multicore.cpp helper.hpp scheduler.hpp objects.hpp -o exec -lsimgrid -g

compile:
	g++ multinode-multicore.cpp helper.hpp scheduler.hpp objects.hpp -o exec -lsimgrid

run:
	./exec small_platform.xml deployment.xml

run150:
	./exec platform-5.5.6.2-torus.xml deployment-5.5.6.2-torus.xml