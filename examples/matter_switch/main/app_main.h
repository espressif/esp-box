/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#pragma once

/** Major version number (X.x.x) */
#define MATTER_SWITCH_VERSION_MAJOR 1
/** Minor version number (x.X.x) */
#define MATTER_SWITCH_VERSION_MINOR 0
/** Patch version number (x.x.X) */
#define MATTER_SWITCH_VERSION_PATCH 0

/**
 * Macro to convert version number into an integer
 *
 * To be used in comparisons, such as MATTER_SWITCH_VERSION >= MATTER_SWITCH_VERSION_VAL(4, 0, 0)
 */
#define MATTER_SWITCH_VERSION_VAL(major, minor, patch) ((major << 16) | (minor << 8) | (patch))

/**
 * Current version, as an integer
 *
 * To be used in comparisons, such as MATTER_SWITCH_VERSION >= MATTER_SWITCH_VERSION_VAL(4, 0, 0)
 */
#define MATTER_SWITCH_VERSION MATTER_SWITCH_VERSION_VAL(MATTER_SWITCH_VERSION_MAJOR, \
                                              MATTER_SWITCH_VERSION_MINOR, \
                                              MATTER_SWITCH_VERSION_PATCH)
