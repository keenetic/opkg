#!/usr/bin/python3

import os, subprocess
import cfg
vardir=os.environ['VARDIR']
debug_opkg_cmds=bool(os.environ.get("DEBUG_OPKG_CMDS") or False)

def opkgcl(opkg_args):
	cmd = "{} -o {} {}".format(cfg.opkgcl, cfg.offline_root, opkg_args)
	if debug_opkg_cmds:
		print("DEBUG cmd: {}".format(cmd))
	p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE,
			stderr=subprocess.STDOUT)
	(stdout_data, stderr_data) = p.communicate()
	if debug_opkg_cmds:
		print("DEBUG stdout: {}".format(stdout_data.decode("utf-8") if stdout_data else None))
		print("DEBUG stderr: {}".format(stderr_data.decode("utf-8") if stderr_data else None))
	status = p.returncode
	return (status, stdout_data.decode("utf-8"))

def install(pkg_name, flags=""):
	return opkgcl("{} --force-postinstall install {}".format(flags, pkg_name))[0]

def remove(pkg_name, flags=""):
	return opkgcl("{} --force-postinstall remove {}".format(flags, pkg_name))[0]

def update():
	return opkgcl("update")[0]

def upgrade(params=None, flags=""):
	if params:
		return opkgcl("{} --force-postinstall upgrade {}".format(flags, params))[0]
	else:
		return opkgcl("--force-postinstall upgrade")[0]

def info(pkg_name, params=None, flags=""):
	if params:
		return opkgcl("{} --fields {} info {}".format(flags, params, pkg_name))[1]
	else:
		return opkgcl("{} info {}".format(flags, pkg_name))[1]

def distupgrade(params=None, flags=""):
	if params:
		return opkgcl("{} --force-postinstall dist-upgrade {}".format(flags, params))[0]
	else:
		return opkgcl("--force-postinstall dist-upgrade")[0]

def files(pkg_name):
	output = opkgcl("files {}".format(pkg_name))[1]
	return output.split("\n")[1:]

def flag_hold(pkg_name):
	out = opkgcl("flag hold {}".format(pkg_name))
	return out == "Setting hold flag on package {}.".format(pkg_name)

def flag_unpacked(pkg_name):
	out = opkgcl("flag unpacked {}".format(pkg_name))
	return out == "Setting flags for package {} to unpacked.".format(pkg_name)

def is_installed(pkg_name, version=None, flags=""):
	out = opkgcl("{} list_installed {}".format(flags, pkg_name))[1]
	if len(out) == 0 or out.split()[0] != pkg_name:
		return False
	if version and out.split()[2] != version:
		return False
	if not os.path.exists(("{}"+vardir+"/lib/opkg/info/{}.control")\
				.format(cfg.offline_root, pkg_name)):
		return False
	return True

def is_upgradable(pkg_name, version=None):
	out = opkgcl("list_upgradable")[1]

	if len(out) == 0:
		return False

	for line in out.splitlines():
		pkg, *_, new_v = line.split()

		if pkg != pkg_name:
			continue

		if version and new_v != version:
			return False

		return True

	return False

def is_autoinstalled(pkg_name):
	status_path = ("{}"+vardir+"/lib/opkg/status").format(cfg.offline_root)
	if not os.path.exists(status_path):
		return False
	status_file = open(status_path, "r")
	status = status_file.read()
	status_file.close()
	index_start = status.find("Package: {}".format(pkg_name))
	if index_start < 0:
		return False
	index_end = status.find("\n\n", index_start)
	return status.find("Auto-Installed: yes", index_start, index_end) >= 0

if __name__ == '__main__':
	import sys
	(status, output) = opkgcl(" ".join(sys.argv[1:]))
	print(output)
	exit(status)
