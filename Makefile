compile_debug:
	g++ src/multinode-multicore.cpp -o exec -lsimgrid -g

compile:
	g++ src/multinode-multicore.cpp -o exec -lsimgrid
	chmod +x ./scripts/run.sh
	mkdir -p output

run:
	./scripts/run.sh ${SCHED} ${JOB_FILE}

extract_improvements:
	mkdir -p improvements
	python3 ./scripts/extract_improvements.py

learn:
	python3 ./Q-learning/learn.py

evaluate:
	python3 ./scripts/evaluate.py

learn_transformer:
	python3 ./Q-learning/transformer.py