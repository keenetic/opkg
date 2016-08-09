#! /usr/bin/env python3
#
# Issue 152
#
# Create a package 'a' which depends on 'p', where 'p' is provided by packages
# 'b' and 'c'. Ensure that 'a' can be installed and either 'b' or 'c' is
# installed.
#
# This ensures that opkg can select a provider when both providers are
# effectively equal. Neither is preferred, neither is held, neither matches the
# abstract package name, etc.
#
# In opkg v0.2.3 this failed: neither provider was selected and so no provider
# of 'p' was returned.
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Depends="p")
o.add(Package="b", Provides="p")
o.add(Package="c", Provides="p")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("a")
if not opkgcl.is_installed("a"):
    opk.fail("Package 'a' failed to install.")
if not (opkgcl.is_installed("b") or opkgcl.is_installed("c")):
    opk.fail("No provider of 'p' was installed.")
