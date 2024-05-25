compile_debug:
	g++ src/multinode-multicore.cpp src/helper.hpp src/scheduler.hpp src/objects.hpp -o exec -lsimgrid -g

compile:
	g++ src/multinode-multicore.cpp src/helper.hpp src/scheduler.hpp src/objects.hpp -o exec -lsimgrid
	chmod +x ./scripts/run.sh

run:
	./scripts/run.sh ${SCHED} ${JOB_FILE} 