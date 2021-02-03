clean:
	sudo rm -rf tests_build valgrind.log

install:
	python3 prep_tests.py
	mkdir tests_build || true
	pip3 install pyyaml
	temp=$$(cd tests_build && cmake .. && make)
	python3 clean_tests.py

printnames:
	cd .. && python3 name_validator.py

singletest: printnames clean install
	./tests_build/check_tests $(FOO)

alltests: printnames clean install
	python3 runall.py