/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#pragma once

/** Major version number (X.x.x) */
#define LV_DEMO_VERSION_MAJOR 0
/** Minor version number (x.X.x) */
#define LV_DEMO_VERSION_MINOR 0
/** Patch version number (x.x.X) */
#define LV_DEMO_VERSION_PATCH 1

/**
 * Macro to convert version number into an integer
 *
 * To be used in comparisons, such as LV_DEMO_VERSION >= LV_DEMO_VERSION_VAL(4, 0, 0)
 */
#define LV_DEMO_VERSION_VAL(major, minor, patch) ((major << 16) | (minor << 8) | (patch))

/**
 * Current version, as an integer
 *
 * To be used in comparisons, such as LV_DEMO_VERSION >= LV_DEMO_VERSION_VAL(4, 0, 0)
 */
#define LV_DEMO_VERSION LV_DEMO_VERSION_VAL(LV_DEMO_VERSION_MAJOR, \
                                              LV_DEMO_VERSION_MINOR, \
                                              LV_DEMO_VERSION_PATCH)
