import os

opkdir = "/tmp/opk"
offline_root = "/tmp/opkg"
opkgcl = os.getenv('OPKG_PATH', os.path.realpath("../src/opkg"))
