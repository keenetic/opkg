#! /usr/bin/env python3
#
# Test opkg when specifying the version to install via:
#       opkg install <pkg_name>=<pkg_version>
#
# 1.- Create packages 'a(1.0)', 'a(2.0)' and 'a(3.0)'
# 2.- Install 'a=1.0'
# 3.- Upgrade
# 4.- Install 'a=2.0', with --force-downgrade
# 5.- Install 'a=2.0', with --force-reinstall
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Version ="1.0")
o.add(Package="a", Version ="2.0")
o.add(Package="a", Version ="3.0")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("a=1.0")
if not opkgcl.is_installed("a", "1.0"):
	opk.fail("Package 'a', Version '1.0', failed to install.")

opkgcl.upgrade("a")
if not opkgcl.is_installed("a", "3.0"):
	opk.fail("Package 'a' was not upgraded to Version '3.0'.")

opkgcl.install("a=2.0", "--force-downgrade")
if not opkgcl.is_installed("a", "2.0"):
	opk.fail("Package 'a' was not downgraded to Version '2.0'.")

opkgcl.install("a=2.0", "--force-reinstall")
if not opkgcl.is_installed("a", "2.0"):
	opk.fail("Package 'a', Version '2.0 was not re-installed.")
