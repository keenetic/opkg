#! /usr/bin/env python3
#
# Reporter: Graham Gower
#
# If a tarball contains a long symlink (over 100 chars) in a longpath (over 100
# chars) then the resulting link or path can be truncated to 100 chars.
#
# This is due to a bug where if both 'L' and 'K' entries are found in the
# tarball, only the first one takes affect due to get_header_tar recursively
# calling itself. To fix this, process longname and linkname at the end of the
# function rather than the start after any subcalls have taken place.
#
#
# Status
# ======
#
# Fixed in r594.

import os
import opk, cfg, opkgcl

opk.regress_init()

long_dir = 110*"a"
long_b = 110*"b"
long_filename = long_dir + "/"+ long_b
long_filename2 = long_dir + "/" + 110*"c"

os.mkdir(long_dir)
open(long_filename, "w").close()
os.symlink(long_b, long_filename2)
a = opk.Opk(Package="a")
a.write(data_files=[long_dir, long_filename, long_filename2])
os.unlink(long_filename)
os.unlink(long_filename2)
os.rmdir(long_dir)
opkgcl.install("a_1.0_all.opk")

if not opkgcl.is_installed("a"):
	opk.fail("Package 'a' not installed.")

if not os.path.exists("{}/{}".format(cfg.offline_root, long_dir)):
	opk.fail("dir with name longer than 100 "
					"characters not created.")

if not os.path.exists("{}/{}".format(cfg.offline_root, long_filename)):
	opk.fail("file with a name longer than 100 characters, "
				"in dir with name longer than 100 characters, "
				"not created.")

if not os.path.lexists("{}/{}".format(cfg.offline_root, long_filename2)):
	opk.fail("symlink with a name longer than 100 characters, "
				"pointing at a file with a name longer than "
				"100 characters,"
				"in dir with name longer than 100 characters, "
				"not created.")

linky = os.path.realpath("{}/{}".format(cfg.offline_root, long_filename2))
linky_dst = "{}/{}".format(cfg.offline_root, long_filename)
if linky != linky_dst:
	opk.fail("symlink path truncated.")

opkgcl.remove("a")
