PYTHON := /usr/bin/python3
REGRESSION_TESTS := 01_install.py 02_file.py 03_upgrade.py 04_upgrade_file.py \
			05_downgrade.py 06_dependency.py 07_hold_upgrade.py \
			08_hold_by_hand.py 09_hold_depends.py \
			10_virtual_conflicts_first.py \
			11_virtual_conflicts_second.py \
			12_virtual_no_conflicts.py \
			13_virtual_upgrade.py 14_upgrade_by_uri.py \
			15_reinstall_by_uri.py 16_upgrade_dependency.py \
			17_upgrade_conflicts.py 18_upgrade_recommends.py \
			19_no_recommends.py 20_exclude.py \
			issue26.py issue31.py issue45.py issue46.py \
			issue50.py issue51.py issue55.py issue58.py \
			issue72.py issue79.py issue85.py issue89.py \
			issue91.py issue103.py issue121.py \
			issue124.py issue124a.py issue127.py \
			issue152.py \
			filehash.py update_loses_autoinstalled_flag.py
RUN_TESTS := $(REGRESSION_TESTS:%.py=run-%.py)

regress: $(RUN_TESTS)

run-%.py: %.py
	@echo $^
	@$(PYTHON) $^

clean:
	rm -rf __pycache__ *.pyc

.PHONY: regress clean
