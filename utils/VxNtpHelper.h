/**
 * @file    VxNtpHelper.h
 * <pre>
 * Copyright (c) 2018, Gaaagaa All rights reserved.
 *
 * file name ：VxNtpHelper.h
 * Creation date ：October 19, 2018 
 * File identification ：
 * File Summary: Some auxiliary function interfaces and related data definitions when using the NTP protocol. 
 *
 * current version ：1.0.0.0
 * Author ：
 * Completion Date ：October 19, 2018 
 * Version summary ：
 *
 * Superseded version ：
 * Original author   ：
 * Completion Date ：
 * Version summary ：
 * </pre>
 */

#ifndef __VXNTPHELPER_H__
#define __VXNTPHELPER_H__

#include "VxDType.h"

////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////

#ifndef NTP_OUTPUT
#define NTP_OUTPUT 1    ///< The macro switch of whether to output debugging information during the interface call 
#endif // NTP_OUTPUT

#define NTP_PORT   123  ///< NTP Dedicated port number 

/**
 * @struct x_ntp_timestamp_t
 * @brief  NTP Timestamp 。
 */
typedef struct x_ntp_timestamp_t
{
    x_uint32_t  xut_seconds;    ///< The number of seconds since 1900 
    x_uint32_t  xut_fraction;   ///< The fractional part, the unit is 4294.967296 (= 2^32 / 10^6) times the number of microseconds 
} x_ntp_timestamp_t;

/**
 * @struct x_ntp_timeval_t
 * @brief  Redefine the system's timeval structure 。
 */
typedef struct x_ntp_timeval_t
{
    x_long_t    tv_sec;    ///< second 
    x_long_t    tv_usec;   ///< Microsecond 
} x_ntp_timeval_t;

/**
 * @struct x_ntp_time_context_t
 * @brief  Time description information structure 。
 */
typedef struct x_ntp_time_context_t
{
    x_uint32_t   xut_year   : 16;  ///< year 
    x_uint32_t   xut_month  :  6;  ///< Month
    x_uint32_t   xut_day    :  6;  ///< day 
    x_uint32_t   xut_week   :  4;  ///< week
    x_uint32_t   xut_hour   :  6;  ///< hour
    x_uint32_t   xut_minute :  6;  ///< minute 
    x_uint32_t   xut_second :  6;  ///< second 
    x_uint32_t   xut_msec   : 14;  ///< millisecond 
} x_ntp_time_context_t;

////////////////////////////////////////////////////////////////////////////////

/**********************************************************/
/**
 * @brief Get the time value of the current system (in 100 nanoseconds, the time from January 1, 1970 to the present) 。
 */
x_uint64_t ntp_gettimevalue(void);

/**********************************************************/
/**
 * @brief Get the timeval value of the current system (the time from January 1, 1970 to the present) 。
 */
x_void_t ntp_gettimeofday(x_ntp_timeval_t * xtm_value);

/**********************************************************/
/**
 * @brief Convert x_ntp_time_context_t to 100 nanoseconds  
 *        Time value in units (the time from January 1, 1970 to the present) 。
 */
x_uint64_t ntp_time_value(x_ntp_time_context_t * xtm_context);

/**********************************************************/
/**
 * @brief Convert the time value (in 100 nanoseconds) (the time from January 1, 1970 to the present) 
 *        For specific time description information (ie x_ntp_time_context_t) 。
 *
 * @param [in ] xut_time    : Time value (the time from January 1, 1970 to the present) 。
 * @param [out] xtm_context : The time description information returned by the operation successfully 。
 *
 * @return x_bool_t
 *         - Success, return X_TRUE ；
 *         - Failure, return X_FALSE 。
 */
x_bool_t ntp_tmctxt_bv(x_uint64_t xut_time, x_ntp_time_context_t * xtm_context);

/**********************************************************/
/**
 * @brief Convert (x_ntp_timeval_t type) time value 
 *        For specific time description information (ie x_ntp_time_context_t) 。
 *
 * @param [in ] xtm_value   : Time value 。
 * @param [out] xtm_context : The time description information returned by the operation successfully 。
 *
 * @return x_bool_t
 *         - Success, return X_TRUE ；
 *         - Failure, return X_FALSE 。
 */
x_bool_t ntp_tmctxt_tv(const x_ntp_timeval_t * const xtm_value, x_ntp_time_context_t * xtm_context);

/**********************************************************/
/**
 * @brief Convert (x_ntp_timeval_t type) time value 
 *        For specific time description information (ie x_ntp_time_context_t) 。
 *
 * @param [in ] xtm_timestamp : Time value 。
 * @param [out] xtm_context   : The time description information returned by the operation successfully 。
 *
 * @return x_bool_t
 *         - Success, return X_TRUE ；
 *         - Failure, return X_FALSE 。
 */
x_bool_t ntp_tmctxt_ts(const x_ntp_timestamp_t * const xtm_timestamp, x_ntp_time_context_t * xtm_context);

////////////////////////////////////////////////////////////////////////////////

/**********************************************************/
/**
 * @brief Send NTP request to NTP server, get server timestamp, for testing 。
 *
 * @param [in ] xszt_host : The IP (four-band IP address) or domain name of the NTP server (such as 3.cn.pool.ntp.org) 。
 * @param [in ] xut_port  : The port number of the NTP server (the default port number NTP_PORT: 123 can be used) 。
 * @param [in ] xut_tmout : The timeout period of the network request (in milliseconds) 。
 * @param [out] xut_timev : The response time value returned by the successful operation (in 100 nanoseconds, the time from January 1, 1970 to the present) 。
 *
 * @return x_int32_t
 *         - Success, return 0 ；
 *         - Failure, return error code 。
 */
x_int32_t ntp_get_time_test(x_cstring_t xszt_host, x_uint16_t xut_port, x_uint32_t xut_tmout, x_uint64_t * xut_timev);

/**********************************************************/
/**
 * @brief Send NTP request to NTP server to get server timestamp 。
 *
 * @param [in ] xszt_host : The IP (four-band IP address) or domain name of the NTP server (such as 3.cn.pool.ntp.org) 。
 * @param [in ] xut_port  : The port number of the NTP server (the default port number NTP_PORT: 123 can be used) 。
 * @param [in ] xut_tmout : The timeout period of the network request (in milliseconds) 。
 * @param [out] xut_timev : The response time value returned by the successful operation (in 100 nanoseconds, the time from January 1, 1970 to the present) 。
 *
 * @return x_int32_t
 *         - Success, return 0 ；
 *         - Failure, return error code 。
 */
x_int32_t ntp_get_time(x_cstring_t xszt_host, x_uint16_t xut_port, x_uint32_t xut_tmout, x_uint64_t * xut_timev);


////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}; // extern "C"
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////

#endif // __VXNTPHELPER_H__
