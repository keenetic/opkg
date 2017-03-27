#! /usr/bin/env python3
#
# Issue 124, reported by muusclaus
#
# This is an additional test case for issue 124 to cover the case where multiple
# packages depend on a common package with version constraints which change
# during an upgrade. The initial bugfix correctly handled the case with a single
# dependent but with multiple dependent packages it became impossible to upgrade
# any of them. This test case has been added to ensure that the final bugfix
# allows all the packages to be upgraded together.

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Version="1.0", Depends="b (= 1.0)")
o.add(Package="a_second", Version="1.0", Depends="b (=1.0)")
o.add(Package="b", Version="1.0")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("a a_second")
if not opkgcl.is_installed("a"):
	opk.fail("Package 'a' failed to install.")
if not opkgcl.is_installed("a_second"):
	opk.fail("Package 'a_second' failed to install.")
if not opkgcl.is_installed("b"):
	opk.fail("Package 'b' not installed despite dependency from package 'a'.")

o = opk.OpkGroup()
o.add(Package="a", Version="1.1", Depends="b (= 1.1)")
o.add(Package="a_second", Version="1.1", Depends="b (= 1.1)")
o.add(Package="b", Version="1.1")
o.write_opk()
o.write_list()

opkgcl.update()

# 'opkg upgrade b' should fail as it won't upgrade a or a_second
opkgcl.upgrade("b")

# Check 'a' has not been upgraded
if opkgcl.is_installed("a", "1.1"):
	opk.xfail("[libsolv] Package 'a' upgraded despite not being listed in packages to upgrade.")
if not opkgcl.is_installed("a", "1.0"):
	opk.fail("Package 'a' version 1.0 removed.")

# Check 'a_second' has not been upgraded
if opkgcl.is_installed("a_second", "1.1"):
	opk.fail("Package 'a_second' upgraded despite not being listed in packages to upgrade.")
if not opkgcl.is_installed("a_second", "1.0"):
	opk.fail("Package 'a_second' version 1.0 removed.")

# Check 'b' has not been upgraded
if opkgcl.is_installed("b", "1.1"):
	opk.fail("Package 'b' upgraded despite breaking dependency of package 'a'.")
if not opkgcl.is_installed("b", "1.0"):
	opk.fail("Package 'b' version 1.0 removed.")

# 'opkg upgrade a' should fail as it won't upgrade a_second
opkgcl.upgrade("a")

# Check 'a' has not been upgraded
if opkgcl.is_installed("a", "1.1"):
	opk.fail("Package 'a' upgraded despite not being listed in packages to upgrade.")
if not opkgcl.is_installed("a", "1.0"):
	opk.fail("Package 'a' version 1.0 removed.")

# Check 'a_second' has not been upgraded
if opkgcl.is_installed("a_second", "1.1"):
	opk.fail("Package 'a_second' upgraded despite not being listed in packages to upgrade.")
if not opkgcl.is_installed("a_second", "1.0"):
	opk.fail("Package 'a_second' version 1.0 removed.")

# Check 'b' has not been upgraded
if opkgcl.is_installed("b", "1.1"):
	opk.fail("Package 'b' upgraded despite breaking dependency of package 'a'.")
if not opkgcl.is_installed("b", "1.0"):
	opk.fail("Package 'b' version 1.0 removed.")

# 'opkg upgrade --combine a a_second' should succeed and upgrade a, a_second and b
opkgcl.upgrade("--combine a a_second")

# Check 'a' has been upgraded
if not opkgcl.is_installed("a", "1.1"):
	opk.fail("Package 'a' failed to upgrade.")
if opkgcl.is_installed("a", "1.0"):
	opk.fail("Package 'a' version 1.0 not removed despite successful upgrade.")

# Check 'a_second' has been upgraded
if not opkgcl.is_installed("a_second", "1.1"):
	opk.fail("Package 'a_second' failed to upgrade.")
if opkgcl.is_installed("a_second", "1.0"):
	opk.fail("Package 'a_second' version 1.0 not removed despite successful upgrade.")

# Check 'b' has been upgraded
if not opkgcl.is_installed("b", "1.1"):
	opk.fail("Package 'b' failed to upgrade.")
if opkgcl.is_installed("b", "1.0"):
	opk.fail("Package 'b' version 1.0 not removed despite successful upgrade.")
