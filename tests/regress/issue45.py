#! /usr/bin/env python3
#
# Reporter: pixdamix
#
# What steps will reproduce the problem?
# ======================================
#
# 1. Extract two package package-a, package-b (wich depends on package-a)
# 2. Under some condition when package are flagged as extracted
#    opkg configure will configure package-b before package-a
#
#
# What is the expected output? What do you see instead?
# =====================================================
#
# opkg should configure package-a before package-b
#
#
# Status
# ======
#
# pixdamix:
# > Resolved in r521

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Depends="b")
o.add(Package="b")
o.write_opk()
o.write_list()

opkgcl.update()

(status, output) = opkgcl.opkgcl("install --force-postinstall a")
ln_a = output.find("Configuring a")
ln_b = output.find("Configuring b")

if ln_a == -1:
	opk.fail("Didn't see package 'a' get configured.")

if ln_b == -1:
	opk.fail("Didn't see package 'b' get configured.")

if ln_a < ln_b:
	opk.fail("Packages 'a' and 'b' configured in wrong order.")

opkgcl.remove("a")
opkgcl.remove("b")
