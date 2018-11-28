#! /usr/bin/env python3
#
# Create package 'a(1.0)' with 2 other fields.
# Check that info command reports all fields without a filter.
# Check that info command reports specified fields only with a filter.
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Version="1.0", Description="d", Section="s")
o.write_opk()
o.write_list()

opkgcl.update()

info = opkgcl.info("a")
if "Package" not in info:
        opk.fail(info)
if "Version" not in info:
        opk.fail("Package 'a' information should contain 'Version' field")
if "Description" not in info:
        opk.fail("Package 'a' information should contain 'Description' field")
if "Section" not in info:
        opk.fail("Package 'a' information should contain 'Section' field")
if "Essential" in info:
        opk.fail("Package 'a' information should not contain 'Essential' field because it is not in the package")

info = opkgcl.info("a", "Version,Description,SomethingElse")
if "Package" not in info:
        opk.fail("Package 'a' information should contain 'Package' field even if it's not in the filter")
if "Version" not in info:
        opk.fail("Package 'a' information should contain 'Version' field because it is in the filter")
if "Description" not in info:
        opk.fail("Package 'a' information should contain 'Description' field because it is in the filter")
if "Section" in info:
        opk.fail("Package 'a' information should not contain 'Section' field because it is not in the filter")

info = opkgcl.info("a", "SomethingElse")
if "Package" not in info:
        opk.fail("Package 'a' information should contain 'Package' field even if it's not in the filter")
if "Version" in info:
        opk.fail("Package 'a' information should not contain 'Version' field because it is not in the filter")
if "Description" in info:
        opk.fail("Package 'a' information should not contain 'Description' field because it is not in the filter")
if "Section" in info:
        opk.fail("Package 'a' information should not contain 'Section' field because it is not in the filter")
