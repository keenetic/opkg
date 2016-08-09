#! /usr/bin/env python3

import os
import opk, cfg, opkgcl

opk.regress_init()

open("asdf", "w").close()
a = opk.Opk(Package="a", Version="1.0", Architecture="all")
a.write(data_files=["asdf"])
b = opk.Opk(Package="b", Version="1.0", Architecture="all")
b.write(data_files=["asdf"])
os.unlink("asdf")
opkgcl.install("a_1.0_all.opk")

if not opkgcl.is_installed("a"):
	opk.fail("Package 'a' not installed.")

if not os.path.exists("{}/asdf".format(cfg.offline_root)):
	opk.fail("asdf not created.")

opkgcl.install("b_1.0_all.opk", "--force-overwrite")

if "{}/asdf".format(cfg.offline_root) not in opkgcl.files("b"):
	opk.fail("asdf not claimed by ``b''.")

if "{}/asdf".format(cfg.offline_root) in opkgcl.files("a"):
	opk.fail("asdf is still claimed by ``a''.")

opkgcl.remove("b")
opkgcl.remove("a")
