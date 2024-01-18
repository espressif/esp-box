#!/bin/bash

rename_file() {
    local file="$1"
    local version_tag="$2"
    local version_file="$3"

    version=$(grep -E "#define ${version_tag}_(MAJOR|MINOR|PATCH)" "$version_file" | awk '{print $3}')
    version="${version//[$'\t\r\n ']/}"
    version_with_dots=$(echo "$version" | sed 's/\(.\)/\1_/g' | sed 's/\_$//')

    case "$file" in
        *_sdkconfig.ci.box-3*)
            newfile=$(echo "$file" | sed "s/_sdkconfig.ci.box-3/-ESP-BOX-3-$version_with_dots/g")
            ;;
        *_sdkconfig.ci.box-lite*)
            newfile=$(echo "$file" | sed "s/_sdkconfig.ci.box-lite/-ESP-BOX-Lite-$version_with_dots/g")
            ;;
        *_sdkconfig.ci.box*)
            newfile=$(echo "$file" | sed "s/_sdkconfig.ci.box/-ESP-BOX-$version_with_dots/g")
            ;;
        *)
            newfile="$file"
            ;;
    esac

    mv "$file" "$newfile"
}

for file in *; do
    if [[ $file == *chatgpt_demo_sdkconfig.ci* ]]; then
        rename_file "$file" "CHATGPT_DEMO_VERSION" "$@/examples/chatgpt_demo/main/main.h"
    elif [[ $file == *factory_demo_sdkconfig.ci* ]]; then
        rename_file "$file" "BOX_DEMO_VERSION" "$@/examples/factory_demo/main/main.h"
    elif [[ $file == *usb_headset_sdkconfig.ci* ]]; then
        rename_file "$file" "USB_HEADSET_VERSION" "$@/examples/usb_headset/main/main.h"
    elif [[ $file == *usb_camera_lcd_display_sdkconfig.ci* ]]; then
        rename_file "$file" "USB_CAMERA_VERSION" "$@/examples/usb_camera_lcd_display/main/main.h"
    elif [[ $file == *matter_switch_sdkconfig.ci* ]]; then
        rename_file "$file" "MATTER_SWITCH_VERSION" "$@/examples/matter_switch/main/app_main.h"
    elif [[ $file == *image_display_sdkconfig.ci* ]]; then
        rename_file "$file" "IMAGE_DISPLAY_VERSION" "$@/examples/image_display/main/image_display.h"
    elif [[ $file == *lv_demos_sdkconfig.ci* ]]; then
        rename_file "$file" "LV_DEMO_VERSION" "$@/examples/lv_demos/main/lv_demos.h"
    elif [[ $file == *mp3_demo_sdkconfig.ci* ]]; then
        rename_file "$file" "MP3_DEMO_VERSION" "$@/examples/mp3_demo/main/mp3_demo.h"
    elif [[ $file == *watering_demo_sdkconfig.ci* ]]; then
        rename_file "$file" "WATERING_DEMO_VERSION" "$@/examples/watering_demo/main/main.h"
    fi
done
