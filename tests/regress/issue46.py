#!/usr/bin/python3

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Version="1.0", Recommends="b")
o.add(Package="b", Version="2.0")
o.write_opk()
o.write_list()

# prime the status file so 'b' is not installed as a recommendation
status_filename = "{}/usr/lib/opkg/status".format(cfg.offline_root)
f = open(status_filename, "w")
f.write("Package: b\n")
f.write("Version: 1.0\n")
f.write("Architecture: all\n")
f.write("Status: deinstall hold not-installed\n")
f.close()

opkgcl.update()

opkgcl.install("a")
if opkgcl.is_installed("b"):
	opk.fail("Package 'b' installed despite "
					"deinstall/hold status.")

opkgcl.remove("a")
opkgcl.install("a")
if opkgcl.is_installed("b"):
	opk.fail("Package 'b' installed - deinstall/hold status "
					"not retained.")

opkgcl.remove("a")
open(status_filename, "w").close()
