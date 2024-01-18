/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#pragma once

/** Major version number (X.x.x) */
#define IMAGE_DISPLAY_VERSION_MAJOR 0
/** Minor version number (x.X.x) */
#define IMAGE_DISPLAY_VERSION_MINOR 0
/** Patch version number (x.x.X) */
#define IMAGE_DISPLAY_VERSION_PATCH 1

/**
 * Macro to convert version number into an integer
 *
 * To be used in comparisons, such as IMAGE_DISPLAY_VERSION >= IMAGE_DISPLAY_VERSION_VAL(4, 0, 0)
 */
#define IMAGE_DISPLAY_VERSION_VAL(major, minor, patch) ((major << 16) | (minor << 8) | (patch))

/**
 * Current version, as an integer
 *
 * To be used in comparisons, such as IMAGE_DISPLAY_VERSION >= IMAGE_DISPLAY_VERSION_VAL(4, 0, 0)
 */
#define IMAGE_DISPLAY_VERSION IMAGE_DISPLAY_VERSION_VAL(IMAGE_DISPLAY_VERSION_MAJOR, \
                                              IMAGE_DISPLAY_VERSION_MINOR, \
                                              IMAGE_DISPLAY_VERSION_PATCH)
