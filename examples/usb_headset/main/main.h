/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#pragma once

/** Major version number (X.x.x) */
#define USB_HEADSET_VERSION_MAJOR 0
/** Minor version number (x.X.x) */
#define USB_HEADSET_VERSION_MINOR 1
/** Patch version number (x.x.X) */
#define USB_HEADSET_VERSION_PATCH 1

/**
 * Macro to convert version number into an integer
 *
 * To be used in comparisons, such as USB_HEADSET_VERSION >= USB_HEADSET_VERSION_VAL(4, 0, 0)
 */
#define USB_HEADSET_VERSION_VAL(major, minor, patch) ((major << 16) | (minor << 8) | (patch))

/**
 * Current version, as an integer
 *
 * To be used in comparisons, such as USB_HEADSET_VERSION >= USB_HEADSET_VERSION_VAL(4, 0, 0)
 */
#define USB_HEADSET_VERSION USB_HEADSET_VERSION_VAL(USB_HEADSET_VERSION_MAJOR, \
                                              USB_HEADSET_VERSION_MINOR, \
                                              USB_HEADSET_VERSION_PATCH)
