#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
#
# Reporter: Paul Spangler <paul.spangler@ni.com>
#
# What steps will reproduce the problem?
########################################
#
# 1. Create a sub-directory 'ipks' beneath the root of the feed.
# 2. Create package 'a' in the 'ipks/' directory.
# 3. Create a Package file at the feed root with an entry for package 'a'.
# 4. Set the 'Filename' attribute for 'a' to its relative path under the
#    subdirectory, like: './ipks/a_1.0.ipk'
# 5. From a CWD outside of the feed root, `opkg download a`
#
# What is the expected output? What do you see instead?
########################################
# Expected:
# Package 'a' is downloaded to the current working directory, with a relative
# path like './a_1.0.ipk'.
#
# Regressive Behavior:
# Package 'a' is downloaded to the full, relative path asserted by 'Filename'.
# opkg throws an error if the directory tree does not exist, relative to the
# current working directory.

import os
import opk, cfg, opkgcl
from shutil import rmtree
from pathlib import Path

opk.regress_init()

o = opk.OpkGroup()
o.add(Package='a', subdirectory='./ipks/')
o.write_opk()
o.write_list()
opkgcl.update()

workdir = Path(cfg.opkdir, 'work')
# refresh the work directory, clearing old downloads
try:
    rmtree(workdir)
except FileNotFoundError: pass
os.makedirs(workdir, exist_ok=False)
os.chdir(workdir)

# check that the relative-pathed package can be installed
opkgcl.install('a')
if not opkgcl.is_installed('a'):
    opk.fail("Relatively-pathed package 'a' reports as not installed.")
opkgcl.remove('a')

# check that a download operation deposits the file in work/, rather than a
# subdirectory.
status = opkgcl.download('a')
if status > 0:
    opk.fail('opkg download operation returned status code %d' % status)

path_actual = os.path.join(workdir, 'a_1.0_all.opk')
try:
    os.stat(path_actual)
except FileNotFoundError:
    opk.fail("Package 'a' downloaded to an incorrect path.")