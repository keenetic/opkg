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
		    core/21_autoremove.py \
		    core/22_autoremove_dependency.py \
		    core/23_force_depends.py \
		    core/24_nodeps.py \
		    core/25_essential.py \
		    core/26_prefer_arch_to_version.py \
		    core/27_reldeps_lt.py \
		    core/28_reldeps_lteq.py \
		    core/29_reldeps_eq.py \
		    core/30_reldeps_gteq.py \
		    core/31_reldeps_gt.py \
		    core/32_intercept_disable.py \
		    core/33_install_exact_version.py \
		    core/34_list_upgradable.py \
		    core/35_download_only.py \
		    core/36_install_virtual.py \
		    core/37_globs.py \
		    core/38_install_constrained_version.py \
		    core/39_distupgrade.py \
		    regress/issue26.py \
		    regress/issue31.py \
		    regress/issue32.py \
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
		    regress/issue156.py \
		    regress/issue162.py \
		    regress/issue163.py \
		    regress/issue165.py \
		    regress/issue165a.py \
		    regress/issue165b.py \
		    regress/issue167.py \
		    regress/issue8601.py \
		    regress/issue8912.py \
		    regress/issue8913.py \
		    regress/issue9059.py \
		    regress/issue9218.py \
		    regress/issue9533.py \
		    regress/issue9802.py \
		    regress/issue9802b.py \
		    regress/issue9987.py \
		    regress/issue9939.py \
		    regress/issue10358.py \
		    regress/issue10358b.py \
		    regress/issue10777.py \
		    regress/issue10781.py \
		    regress/issue11033.py \
		    regress/issue11033b.py \
		    misc/filehash.py \
		    misc/update_loses_autoinstalled_flag.py \
		    misc/version_comparisons.py
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
