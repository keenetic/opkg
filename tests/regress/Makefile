PYTHON := /usr/bin/python3
REGRESSION_TESTS := issue26.py issue31.py issue45.py issue46.py \
			issue50.py issue51.py issue55.py issue58.py \
			issue72.py issue79.py issue84.py issue85.py \
			filehash.py \
			update_loses_autoinstalled_flag.py
RUN_TESTS := $(REGRESSION_TESTS:%.py=run-%.py)

regress: $(RUN_TESTS)

run-%.py: %.py
	@echo $^
	@$(PYTHON) $^

clean:
	rm -rf __pycache__ *.pyc

.PHONY: regress clean
