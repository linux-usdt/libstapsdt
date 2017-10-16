#!/usr/bin/env python3

import os
import re
from os import path
import argparse
import fileinput

def inline_file_edit(filepath):
    return fileinput.FileInput(filepath, inplace=True)

LIB_SHORT_DESCRIPTION = "Runtime USDT probes for Linux"
LIB_LONG_DESCRIPTION = """
Library to give Linux runtime USDT probes capability
""".strip()

HEADERS_SHORT_DESCRIPTION = "Headers for libstapsdt"
HEADERS_LONG_DESCRIPTION = """
Headers for libstapsdt, a library to give Linux runtime USDT probes capability
""".strip()

SITE = "https://github.com/sthima/libstapsdt"

parser = argparse.ArgumentParser()
parser.add_argument("codename", type=str)
parser.add_argument("version", type=str)
args = parser.parse_args()

BASE = path.join("dist", "libstapsdt-{0}".format(args.version))
DEBIAN = path.join(BASE, "debian")

print(args.version)
print(args.codename)

# Rename

os.rename(path.join(DEBIAN, "libstapsdt1.install"), path.join(DEBIAN, "libstapsdt0.install"))
os.rename(path.join(DEBIAN, "libstapsdt1.dirs"), path.join(DEBIAN, "libstapsdt0.dirs"))

# Fix changelog
with inline_file_edit(path.join(DEBIAN, "changelog")) as file_:
    for line in file_:
        if 'unstable' in line:
            line = line.replace('unstable', args.codename)
        elif 'Initial release' in line:
            line = "  * Initial release\n"
        print(line, end="")


# Fix control
header = True
with inline_file_edit(path.join(DEBIAN, "control")) as file_:
    for line in file_:
        if line.startswith("#"):
            continue
        if "3.9.6" in line:
            line = line.replace("3.9.6", "3.9.7")
        if "upstream URL" in line:
            line = line.replace("<insert the upstream URL, if relevant>", SITE)
        if "BROKEN" in line:
            line = line.replace("BROKEN", "0")
        if "debhelper (>=9)" in line:
            line = line.replace("\n", ", libelf1, libelf-dev\n")
        if "insert up" in line:
            if header:
                line = line.replace("<insert up to 60 chars description>", HEADERS_SHORT_DESCRIPTION)
            else:
                line = line.replace("<insert up to 60 chars description>", LIB_SHORT_DESCRIPTION)
        if "insert long" in line:
            if header:
                line = line.replace("<insert long description, indented with spaces>", HEADERS_LONG_DESCRIPTION)
                header = False
            else:
                line = line.replace("<insert long description, indented with spaces>", LIB_LONG_DESCRIPTION)
        print(line, end="")

# Fix copyright
header = True
COPYRIGHT_LINE = ""
copyright_regex = re.compile("Copyright: [0-9]")
with open(path.join(DEBIAN, "copyright")) as file_:
    for line in file_.readlines():
        if copyright_regex.match(line):
            COPYRIGHT_LINE = line

with inline_file_edit(path.join(DEBIAN, "copyright")) as file_:
    for line in file_:
        if line.startswith("#"):
            continue
        if "url://example" in line:
            line = line.replace("<url://example.com>", SITE)
        if "<years>" in line:
            if "Copyright" in line:
                line = COPYRIGHT_LINE
            else:
                continue
        print(line, end="")

# Fix installs
with open(path.join(DEBIAN, "libstapsdt-dev.links"), "w+") as file_:
    file_.writelines(["usr/lib/libstapsdt.so.0 usr/lib/libstapsdt.so"])
with inline_file_edit(path.join(DEBIAN, "libstapsdt0.install")) as file_:
    for line in file_:
        print("usr/lib/lib*.so.*")
        break
with inline_file_edit(path.join(DEBIAN, "libstapsdt-dev.install")) as file_:
    for line in file_:
        print("usr/include/*")
        break

# Fix rules
with inline_file_edit(path.join(DEBIAN, "rules")) as file_:
    for line in file_:
        if "DH_VERBOSE" in line:
            print("export DH_VERBOSE=1")
            print("export DEB_BUILD_OPTIONS=nocheck")
            continue
        else:
            print(line, end="")
