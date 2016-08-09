#! /usr/bin/env python3
#
# Within the function `pkg_hash_fetch_best_installation_candidate`, we wish to
# test the condition where `(held_pkg && good_pkg_by_name) == true`. So install
# a 1.0, mark it as on hold and explicitly install 'a_2.0_all.opk'. This
# installation should be blocked.
#

import os
import opk, cfg, opkgcl

opk.regress_init()

a_10 = opk.Opk(Package="a", Version="1.0")
a_10.write()
a_20 = opk.Opk(Package="a", Version="2.0")
a_20.write()

opkgcl.install("a_1.0_all.opk")
if not opkgcl.is_installed("a"):
	opk.fail("Package 'a' installed but reports as not installed.")

opkgcl.flag_hold("a")

# Try to explicitly install a version 2.0
opkgcl.install("a_2.0_all.opk")

if opkgcl.is_installed("a", "2.0"):
	opk.fail("Old version of package 'a' flagged as on hold but was upgraded.")
if not opkgcl.is_installed("a", "1.0"):
	opk.fail("Package 'a' not upgraded but old version was removed.")
