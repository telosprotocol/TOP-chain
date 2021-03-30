#pragma once

#include <string>
#include "xbase/xcontext.h"

namespace top {

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


#ifdef __linux__
    typedef int32_t          x_sockfd_t;
#else // !__linux__
    typedef size_t           x_sockfd_t;
#endif // __linux__


#define X_INVALID_SOCKFD  ((x_sockfd_t)~0)


#define TEXT_LEN_256      256
#define TEXT_LEN_PATH     260


#define NTP_PORT   123

/**
 * @struct x_ntp_timestamp_t
 * @brief  NTP timestamp
 */
typedef struct x_ntp_timestamp_t
{
    uint32_t  xut_seconds;    ///< seconds since 1900
    uint32_t  xut_fraction;   ///< fraction part,microseconds * 4294.967296( = 2^32 / 10^6 )
} x_ntp_timestamp_t;

/**
 * @struct x_ntp_time_context_t
 * @brief  time data structure
 */
typedef struct x_ntp_time_context_t
{
    uint32_t   xut_year   : 16;
    uint32_t   xut_month  :  6;
    uint32_t   xut_day    :  6;
    uint32_t   xut_week   :  4;
    uint32_t   xut_hour   :  6;
    uint32_t   xut_minute :  6;
    uint32_t   xut_second :  6;
    uint32_t   xut_msec   : 14;
} x_ntp_time_context_t;

////////////////////////////////////////////////////////////////////////////////

 void bv_output(const char *xszt_name, uint64_t xut_time);

/**********************************************************/
/**
 * @brief get current system time in 100ns since 1970.1.1
 */
uint64_t ntp_gettimevalue(void);


/**********************************************************/
/**
 * @brief get current system time in timeval since 1970.1.1
 */
void ntp_gettimeofday(struct timeval *xtm_value);

/**********************************************************/
/**
 * @brief set current system time since 1970.1.1
 * @param [in ] xut_timev : time in 100ns since 1970.1.1
 */
int ntp_settimeofday(uint64_t xut_timev);

/**********************************************************/
/**
 * @brief transform x_ntp_time_context_t to 100ns time since 1970.0.1
 */
uint64_t ntp_time_value(x_ntp_time_context_t * xtm_context);

/**********************************************************/
/**
 * @brief transform 100ns time since 1970.1.1 to human readable time format
 *
 * @param [in ] xut_time    : 100ns time since 1970.1.1
 * @param [out] xtm_context : readable time data structure
 *
 * @return bool
 */
bool ntp_tmctxt_bv(uint64_t xut_time, x_ntp_time_context_t * xtm_context);

/**********************************************************/
/**
 * @brief transform timeval to human readable time format
 *
 * @param [in ] xtm_value   : time value
 * @param [out] xtm_context : readable time data structure
 *
 * @return bool
 */
bool ntp_tmctxt_tv(const struct timeval *xtm_value, x_ntp_time_context_t * xtm_context);

/**********************************************************/
/**
 * @brief transform ntp timestamp to human readable time format
 *
 * @param [in ] xtm_timestamp : time value
 * @param [out] xtm_context   : readable time data structure
 *
 * @return bool
 */
bool ntp_tmctxt_ts(const x_ntp_timestamp_t * const xtm_timestamp, x_ntp_time_context_t * xtm_context);

////////////////////////////////////////////////////////////////////////////////

/**********************************************************/
/**
 * @brief send NTP request to NTP server
 *
 * @param [in ] xszt_host : NTP ip or host(3.cn.pool.ntp.org)
 * @param [in ] xut_port  : NTP port (default 123)
 * @param [in ] xut_tmout : request timeout in milliseconds
 * @param [out] xut_timev : 100ns time since 1970.1.1
 *
 * @return int32_t 0 on success, error code on failure
 */
int32_t ntp_get_time(const char *xszt_host, uint16_t xut_port, uint32_t xut_tmout, uint64_t * xut_timev);

////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}; // extern "C"
#endif // __cplusplus


}
