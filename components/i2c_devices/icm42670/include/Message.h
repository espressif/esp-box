/*
 * ________________________________________________________________________________________________________
 * Copyright (c) 2015-2015 InvenSense Inc. All rights reserved.
 *
 * This software, related documentation and any modifications thereto (collectively “Software”) is subject
 * to InvenSense and its licensors' intellectual property rights under U.S. and international copyright
 * and other intellectual property rights laws.
 *
 * InvenSense and its licensors retain all intellectual property and proprietary rights in and to the Software
 * and any use, reproduction, disclosure or distribution of the Software without an express license agreement
 * from InvenSense is strictly prohibited.
 *
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE SOFTWARE IS
 * PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * INVENSENSE BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
 * ________________________________________________________________________________________________________
 */

/** @defgroup Message Message
 *  @brief    Utility functions to display and redirect diagnostic messages
 *
 *            Use INV_MSG_DISABLE or INV_MSG_ENABLE define before including
 *            this header to enable/disable messages for a compilation unit.
 *
 *            Under Linux, Windows or Arduino, messages are enabled by default.
 *            Use INV_MSG_DISABLE to disable them.
 *
 *            Under orther environmment, message are disabled by default. 
 *            Use INV_MSG_ENABLE to disable them.
 *
 *  @ingroup  EmbUtils
 *  @{
 */

#ifndef _INV_MESSAGE_H_
#define _INV_MESSAGE_H_

#include "InvExport.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>

/** @brief For eMD target, disable log by default
 *	If  compile switch is set for a compilation unit
 *	messages will be totally disabled by default
 */
#if !defined(__linux) && !defined(_WIN32) && !defined(ARDUINO)
	#define INV_MSG_DISABLE	1
#endif

#define INV_MSG_ENABLE

/** @brief Allow to force enabling messaging using INV_MSG_ENABLE define  */
#ifdef INV_MSG_ENABLE
	#undef INV_MSG_DISABLE
#endif


/** @brief Helper macro for calling inv_msg()
 *	If INV_MSG_DISABLE compile switch is set for a compilation unit
 *	messages will be totally disabled
 */
#define INV_MSG(level, ...) 	      _INV_MSG(level, __VA_ARGS__)

/** @brief Helper macro for calling inv_msg_setup()
 *	If INV_MSG_DISABLE compile switch is set for a compilation unit
 *	messages will be totally disabled
 */
#define INV_MSG_SETUP(level, printer) _INV_MSG_SETUP(level, printer)

/** @brief Helper macro for calling inv_msg_setup_level()
 *	If INV_MSG_DISABLE compile switch is set for a compilation unit
 *	messages will be totally disabled
 */
#define INV_MSG_SETUP_LEVEL(level) 	  _INV_MSG_SETUP_LEVEL(level)

/** @brief Helper macro for calling inv_msg_setup_default()
 *	If INV_MSG_DISABLE compile switch is set for a compilation unit
 *	messages will be totally disabled
 */
#define INV_MSG_SETUP_DEFAULT()       _INV_MSG_SETUP_DEFAULT()

/** @brief Return current level
 *	@warning This macro may expand as a function call
 */
#define INV_MSG_LEVEL                 _INV_MSG_LEVEL

#if defined(INV_MSG_DISABLE)
	#define _INV_MSG(level, ...)           (void)0
	#define _INV_MSG_SETUP(level, printer) (void)0
	#define _INV_MSG_SETUP_LEVEL(level)    (void)0
	#define _INV_MSG_LEVEL                 INV_MSG_LEVEL_OFF
#else
	#define _INV_MSG(level, ...)           inv_msg(level, __VA_ARGS__)
 	#define _INV_MSG_SETUP(level, printer) inv_msg_setup(level, printer)
 	#define _INV_MSG_SETUP_LEVEL(level)    inv_msg_setup(level, inv_msg_printer_default)
	#define _INV_MSG_SETUP_DEFAULT()       inv_msg_setup_default()
	#define _INV_MSG_LEVEL                 inv_msg_get_level()
#endif

/** @brief message level definition
 */
enum inv_msg_level {
	INV_MSG_LEVEL_OFF     = 0,
	INV_MSG_LEVEL_ERROR,
	INV_MSG_LEVEL_WARNING,
	INV_MSG_LEVEL_INFO,
	INV_MSG_LEVEL_VERBOSE,
	INV_MSG_LEVEL_DEBUG,
	INV_MSG_LEVEL_MAX
};


/** @brief Prototype for print routine function
 */
typedef void (*inv_msg_printer_t)(int level, const char * str, va_list ap);


/** @brief Set message level and printer function
 *  @param[in] level   only message above level will be passed to printer function
 *  @param[in] printer user provided function in charge printing message
 *  @return none
 */
void INV_EXPORT inv_msg_setup(int level, inv_msg_printer_t printer);


/** @brief Default printer function that display messages to stderr
 *  Function uses stdio. Care must be taken on embeded platfrom.
 *  Function does nothing with IAR compiler.
 *  @return none
 */
void INV_EXPORT inv_msg_printer_default(int level, const char * str, va_list ap);

/** @brief Set message level
 *  Default printer function will be used.
 *  @param[in] level   only message above level will be passed to printer function
 *  @return none
 */
static inline void inv_msg_setup_level(int level)
{
	inv_msg_setup(level, inv_msg_printer_default);
}


/** @brief Set default message level and printer
 *  @return none
 */
static inline void inv_msg_setup_default(void)
{
	inv_msg_setup(INV_MSG_LEVEL_INFO, inv_msg_printer_default);
}

/** @brief Return current message level
 *  @return current message level
 */
int INV_EXPORT inv_msg_get_level(void);

/** @brief Display a message (through means of printer function)
 *  @param[in] 	level for the message
 *  @param[in] 	str   message string
 *  @param[in] 	...   optional arguments
 *  @return none
 */
void INV_EXPORT inv_msg(int level, const char * str, ...);


#ifdef __cplusplus
}
#endif

#endif /* INV_MESSAGE_H_ */

/** @} */
