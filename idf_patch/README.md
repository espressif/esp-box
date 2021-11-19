# Patch for ESP-IDF

Since some bug fixes of esp-idf may not be synced to GitHub, you need to manually apply some patches to build the example.

## Apply Patch

For Linux and macOS, you can apply the patch by entering the following command on the command line / terminal:

```shell
cd /path/to/esp-box/idf_patch
python3 apply_patch.py -d /path/to/esp-idf
. /path/to/esp-idf/export.sh
```

For Windows users, you may need to do the following manually:

- Change the working directory to the folder where esp-idf is located
- Run the following commands in cmd in sequence：
  - `git fetch origin`
  - `git checkout 35b20cadce65ce79c14cf2018efc87c44d71ab21`
  - `git apply X:\path\to/esp-box\idf_patch\idf_patch.patch`
- Copy the files in the `idf_patch\components\esp_phy` folder and replace them to the same location under esp-idf in turn
- Run `install.bat` in the esp-idf directory
- Run `export.bat` in the esp-idf directory
- Enter the esp-box repo to compile the project you want

## Compile Example

The project provides `sdkconfig.defaults.cn` and `sdkconfig.defaults.en`, which are the default configuration files corresponding to Chinese and English respectively. By replacing `sdkconfig.defaults` with the above file, deleting `sdkconfig`, rebuilding and burning, you can burn routines in the specified language to ESP-Box.
