# SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: CC0-1.0
import io
import os
import argparse
import shutil
import math
import sys

sys.dont_write_bytecode = True

def copy_files(nvs_src_file, nvs_dst_file):
    dst_folder = os.path.dirname(nvs_dst_file)

    if not os.path.exists(dst_folder):
        os.makedirs(dst_folder)

    shutil.copyfile(nvs_src_file, nvs_dst_file)
    print(f"File copied from {nvs_src_file} to {nvs_dst_file}")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='move uf2 fw')
    parser.add_argument('-d1', '--nvs_src_file')
    parser.add_argument('-d2', '--nvs_dst_file')
    args = parser.parse_args()

    if os.path.exists(args.nvs_src_file):
        copy_files(args.nvs_src_file, args.nvs_dst_file)
