PYTHON := /usr/bin/python3
REGRESSION_TESTS := core/01_install.py \
		    core/02_file.py \
		    core/03_upgrade.py \
		    core/04_upgrade_file.py \
		    core/05_downgrade.py \
		    core/06_dependency.py \
		    core/07_hold_upgrade.py \
		    core/08_hold_by_hand.py \
		    core/09_hold_depends.py \
		    core/10_virtual_conflicts_first.py \
		    core/11_virtual_conflicts_second.py \
		    core/12_virtual_no_conflicts.py \
		    core/13_virtual_upgrade.py \
		    core/14_upgrade_by_uri.py \
		    core/15_reinstall_by_uri.py \
		    core/16_upgrade_dependency.py \
		    core/17_upgrade_conflicts.py \
		    core/18_upgrade_recommends.py \
		    core/19_no_recommends.py \
		    core/20_exclude.py \
		    regress/issue26.py \
		    regress/issue31.py \
		    regress/issue45.py \
		    regress/issue46.py \
		    regress/issue50.py \
		    regress/issue51.py \
		    regress/issue55.py \
		    regress/issue58.py \
		    regress/issue72.py \
		    regress/issue79.py \
		    regress/issue85.py \
		    regress/issue89.py \
		    regress/issue91.py \
		    regress/issue103.py \
		    regress/issue121.py \
		    regress/issue124.py \
		    regress/issue124a.py \
		    regress/issue127.py \
		    regress/issue152.py \
		    regress/issue154.py \
		    misc/filehash.py \
		    misc/update_loses_autoinstalled_flag.py
RUN_TESTS := $(REGRESSION_TESTS:%.py=run-%.py)

regress: $(RUN_TESTS)

run-core/%.py: core/%.py
	@echo $^
	@PYTHONPATH=. $(PYTHON) $^

run-regress/%.py: regress/%.py
	@echo $^
	@PYTHONPATH=. $(PYTHON) $^

run-misc/%.py: misc/%.py
	@echo $^
	@PYTHONPATH=. $(PYTHON) $^

clean:
	rm -rf __pycache__ *.pyc

.PHONY: regress clean
