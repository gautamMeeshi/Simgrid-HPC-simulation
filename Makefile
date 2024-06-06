compile_debug:
	g++ src/multinode-multicore.cpp src/helper.hpp src/scheduler.hpp src/objects.hpp -o exec -lsimgrid -g

compile:
	g++ src/multinode-multicore.cpp src/helper.hpp src/scheduler.hpp src/objects.hpp -o exec -lsimgrid
	chmod +x ./scripts/run.sh
	mkdir output

run:
	./scripts/run.sh ${SCHED} ${JOB_FILE}

extract_improvements:
	python3 ./scripts/extract_improvements.py

learn:
	python3 ./Q-learning/learn.py

evaluate:
	python3 ./scripts/evaluate.py