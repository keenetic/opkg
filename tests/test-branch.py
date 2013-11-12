#! /usr/bin/env python
#
#	test-branch.py: 'make check' on each commit on a branch
#
#	Copyright (C) 2013 Paul Barker
#
#	This program is free software; you can redistribute it and/or modify it
#	under the terms of the GNU General Public License as published by the
#	Free Software Foundation; either version 2, or (at your option) any
#	later version.
#
#	This program is distributed in the hope that it will be useful, but
#	WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#	General Public License for more details.

import subprocess as sp
import os, shutil

def fail(msg):
	print(msg)
	exit(-1)

def rsh_s(cmd):
	data = sp.check_output(cmd, shell=True)
	s = data.decode("utf-8")
	return s

def sh(cmd):
	return sp.call(cmd, shell=True)

def rsh(cmd):
	return sp.check_call(cmd, shell=True)

def read_env(key, default_value):
	value = os.environ.get(key)
	if value:
		return value
	else:
		return default_value

def list_commits(c_from, c_to):
	s = rsh_s("git rev-list %s..%s" % (c_from, c_to))
	commits = s.strip().split("\n")
	commits.reverse()
	return commits

def do_checkout(commit):
	os.chdir(opkg_dir)
	rsh("git checkout %s" % commit)

def do_test():
	os.chdir(opkg_dir)
	rsh("./autogen.sh")
	rsh("./configure %s" % OPKG_CONFIGURE_OPTIONS)
	rsh("make")
	rsh("make check")
	rsh("make DESTDIR=%s install" % install_in_dir)
	#rsh("make distclean")
	#os.chdir(build_dir)
	#rsh("%s/configure %s" % (opkg_dir, OPKG_CONFIGURE_OPTIONS))
	#rsh("make")
	#rsh("make check")
	#rsh("make DESTDIR=%s install" % install_out_dir)

OPKG_REPO = read_env("OPKG_ENV", "https://bitbucket.org/opkg/opkg-staging")
OPKG_MASTER = read_env("OPKG_MASTER", "master")
OPKG_BRANCH = read_env("OPKG_BRANCH", "pbarker/testing")
OPKG_TEST_DIR = read_env("OPKG_TEST_DIR", "/tmp/opkg-test")
OPKG_CONFIGURE_OPTIONS = read_env("OPKG_CONFIGURE_OPTIONS", "--enable-sha256")

repo_dir = os.getcwd()
commits = list_commits(OPKG_MASTER, OPKG_BRANCH)

for c in commits:
	commit_dir = os.path.join(OPKG_TEST_DIR, c)
	opkg_dir = os.path.join(commit_dir, "opkg")
	build_dir = os.path.join(commit_dir, "build")
	install_in_dir = os.path.join(commit_dir, "install_in")
	install_out_dir = os.path.join(commit_dir, "install_out")

	os.makedirs(opkg_dir, exist_ok=True)
	os.makedirs(build_dir, exist_ok=True)
	os.makedirs(install_in_dir, exist_ok=True)
	os.makedirs(install_out_dir, exist_ok=True)

	rsh("git clone %s %s" % (repo_dir, opkg_dir))
	do_checkout(c)
	do_test()
