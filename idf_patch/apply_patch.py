#!/usr/bin/env python3

import argparse
import os
import platform
import shutil

github_src_commit = "35b20cadce65ce79c14cf2018efc87c44d71ab21"
patch_path = "/idf_patch.patch"
binary_patch_list = {
    "/components/esp_phy/Kconfig",
    "/components/esp_phy/lib/esp32s3/libbtbb.a",
    "/components/esp_phy/lib/esp32s3/libphy.a"}

if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog = "ESP-IDF Patch Apply Script")
    parser.add_argument("--directory", "-d", type = str,  default = None, help = "esp-idf directory")
    args = parser.parse_args()

    if platform.system() == "Windows":
        print("Currently not supported!")
        exit(-1)

    if None == args.directory:
        print("Please input a valid IDF dir")
        exit(-1)

    src_path = os.getcwd()
    idf_path = args.directory

    print("patches at" + src_path)
    print("esp-idf at" + idf_path)
    print("Fetching origin. Please wait...")

    os.chdir(idf_path)
    os.system("git fetch origin")
    os.system("git reset --hard && git clean -d -f -x")
    os.system("git checkout %s" %(github_src_commit))
    os.system("git reset --hard && git clean -d -f -x && git submodule update --init --recursive --force")
    os.system("git apply %s" % (src_path + patch_path))
    os.system("./install.sh")

    for i in binary_patch_list:
        src = src_path + i
        dst = idf_path + i
        print("Copy", src, "to", dst)
        shutil.copy(src, dst)
