#! /usr/bin/env python3
#
# Reporter: paul.spangler@ni.com
#
# What steps will reproduce the problem?
# ======================================
#
# 1.- Create package 'a' which installs a file in
#     in an existing directory. At package creation
#     time set the dir permissions to 0o777
# 2.- Create a rootfs that has the directory where
#     package 'a' will install a file. Set the permisions
#     on that directory to 0o755.
# 3.- Install 'a'
#
# What is the expected output? What do you see instead?
# ======================================
#
# The directory permisions change from 0o755 to 0o777.
# It should stay as 0o755 (dpkg behavior)
#

import os
import opk, cfg, opkgcl

opk.regress_init()

subdir = "dir/subdir"
filename = subdir + "/myfile"
rootfs_subdir = "{}/{}".format(cfg.offline_root, subdir)

os.makedirs(subdir, 0o755)
os.chmod(subdir, 0o777)
open(filename, "w").close()
a = opk.Opk(Package="a")
a.write(data_files=[subdir, filename])
os.unlink(filename)
os.rmdir(subdir)

os.makedirs(rootfs_subdir, 0o755)
opkgcl.install("a_1.0_all.opk")

if not opkgcl.is_installed("a"):
     opk.fail("Package 'a' installed but reports as not installed.")

if os.stat(rootfs_subdir).st_mode != 0o40755:
     opk.fail("Install operation incorrectly changed existing directory permissions")

opkgcl.remove("a")
