#include "xdata/xntp.h"

#include <time.h>

#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <windows.h>
#include <time.h>
#else // !_WIN32
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#endif // _WIN32

#include <string>
#include <vector>

#include "xbase/xbase.h"

namespace top {

static inline void ts_output(const char *xszt_name, const x_ntp_time_context_t * const xtm_ctxt)
{
    xdbg("\t%s : %04d-%02d-%02d_%02d-%02d-%02d.%03d",
        xszt_name           ,
        xtm_ctxt->xut_year  ,
        xtm_ctxt->xut_month ,
        xtm_ctxt->xut_day   ,
        xtm_ctxt->xut_hour  ,
        xtm_ctxt->xut_minute,
        xtm_ctxt->xut_second,
        xtm_ctxt->xut_msec  );
}

void bv_output(const char *xszt_name, uint64_t xut_time)
{
    x_ntp_time_context_t xtm_ctxt;
    ntp_tmctxt_bv(xut_time, &xtm_ctxt);
    ts_output(xszt_name, &xtm_ctxt);
}

#define JAN_1970     0x83AA7E80             ///< seconds between 1900 and 1970
#define NS100_1970   116444736000000000LL   ///< 100ns between 1601 and 1970


typedef enum em_ntp_mode_t
{
    ntp_mode_unknow     = 0,
    ntp_mode_initiative = 1,
    ntp_mode_passive    = 2,
    ntp_mode_client     = 3,
    ntp_mode_server     = 4,
    ntp_mode_broadcast  = 5,
    ntp_mode_control    = 6,
    ntp_mode_reserved   = 7,
} em_ntp_mode_t;

/**
 * @struct x_ntp_packet_t
 * @brief  NTP packet on the wire
 */
typedef struct x_ntp_packet_t
{
    uint8_t         xct_li_ver_mode;      ///< // Eight bits. li, vn, and mode.
                                          ///< li.   Two bits.   Leap indicator.
                                          ///< ver.   Three bits. Version number of the protocol.
                                          ///< mode. Three bits. Client will pick mode 3 for client.

    uint8_t         xct_stratum    ;      ///< Stratum level of the local clock (1-16) 1 highest, 16 unsync
    uint8_t         xct_poll       ;      ///< Maximum interval between successive messages.
    uint8_t         xct_percision  ;      ///< Precision of the local clock.

    uint32_t        xut_root_delay     ;  ///< Total round trip delay time.
    uint32_t        xut_root_dispersion;  ///< Max error aloud from primary clock source.
    uint32_t        xut_ref_indentifier;  ///< Reference clock identifier.

    x_ntp_timestamp_t xtmst_reference;      ///< Reference system timestamp, last set or updated for T1
    x_ntp_timestamp_t xtmst_originate;      ///< Originate timestamp, when request leaves sender for T4
    x_ntp_timestamp_t xtmst_receive  ;      ///< Received timestamp when request reaches receiver for T2
    x_ntp_timestamp_t xtmst_transmit ;      ///< Tramsmit timestamp when response leaves server for T3
} x_ntp_packet_t;

////////////////////////////////////////////////////////////////////////////////

static inline void ntp_timeval_to_timestamp(x_ntp_timestamp_t * xtm_timestamp, const struct timeval * const xtm_timeval)
{
    const double xlft_frac_per_ms = 4.294967296E6;  // 2^32 / 1000

    xtm_timestamp->xut_seconds  = (uint32_t)(xtm_timeval->tv_sec  + JAN_1970);
    xtm_timestamp->xut_fraction = (uint32_t)(xtm_timeval->tv_usec / 1000.0 * xlft_frac_per_ms);
}

static inline void ntp_timestamp_to_timeval(struct timeval * xtm_timeval, const x_ntp_timestamp_t * const xtm_timestamp)
{
    const double xlft_frac_per_ms = 4.294967296E6;  // 2^32 / 1000

    if (xtm_timestamp->xut_seconds >= JAN_1970)
    {
        xtm_timeval->tv_sec  = (long)(xtm_timestamp->xut_seconds - JAN_1970);
        xtm_timeval->tv_usec = (long)(xtm_timestamp->xut_fraction / xlft_frac_per_ms * 1000.0);
    }
    else
    {
        xtm_timeval->tv_sec  = 0;
        xtm_timeval->tv_usec = 0;
    }
}

/**********************************************************/
/**
 * @brief transform struct timeval to 100ns time
 */
static inline uint64_t ntp_timeval_ns100(const struct timeval *xtm_timeval)
{
    return (10000000ULL * xtm_timeval->tv_sec + 10ULL * xtm_timeval->tv_usec);
}

/**********************************************************/
/**
 * @brief transform x_ntp_timestamp_t to 100ns time
 */
static inline uint64_t ntp_timestamp_ns100(const x_ntp_timestamp_t *xtm_timestamp)
{
    struct timeval xmt_timeval;
    ntp_timestamp_to_timeval(&xmt_timeval, xtm_timestamp);
    return ntp_timeval_ns100(&xmt_timeval);
}


uint64_t ntp_gettimevalue(void)
{
#ifdef _WIN32
    FILETIME       xtime_file;
    ULARGE_INTEGER xtime_value;

    GetSystemTimeAsFileTime(&xtime_file);
    xtime_value.LowPart  = xtime_file.dwLowDateTime;
    xtime_value.HighPart = xtime_file.dwHighDateTime;

    return (uint64_t)(xtime_value.QuadPart - NS100_1970);
#else // !_WIN32
    struct timeval tmval;
    gettimeofday(&tmval, nullptr);

    return (10000000ULL * tmval.tv_sec + 10ULL * tmval.tv_usec);
#endif // _WIN32
}

void ntp_gettimeofday(struct timeval *xtm_value)
{
#ifdef _WIN32
    FILETIME       xtime_file;
    ULARGE_INTEGER xtime_value;

    GetSystemTimeAsFileTime(&xtime_file);
    xtime_value.LowPart  = xtime_file.dwLowDateTime;
    xtime_value.HighPart = xtime_file.dwHighDateTime;

    xtm_value->tv_sec  = (long)((xtime_value.QuadPart - NS100_1970) / 10000000LL); // seconds since 1970.1.1
    xtm_value->tv_usec = (long)((xtime_value.QuadPart / 10LL      ) % 1000000LL ); // microsecond aka. us
#else // !_WIN32
    struct timeval tmval;
    gettimeofday(&tmval, nullptr);

    xtm_value->tv_sec  = tmval.tv_sec ;
    xtm_value->tv_usec = tmval.tv_usec;
#endif // _WIN32
}

int ntp_settimeofday(uint64_t xut_timev)
{
#ifdef _WIN32
    return -1;
#else
    struct timeval tmval;
    tmval.tv_sec  = xut_timev / 10000000LL;
    // 100ns
    tmval.tv_usec = xut_timev / 100 % 1000000LL;

    if (tmval.tv_usec < 0) {
        tmval.tv_sec -= 1;
        tmval.tv_usec += 1000000ULL;
    }

    return settimeofday(&tmval, nullptr);


#endif // _WIN32
}

uint64_t ntp_time_value(x_ntp_time_context_t * xtm_context)
{
    uint64_t xut_time = 0ULL;

#if 0
    if ((xtm_context->xut_year   < 1970) ||
        (xtm_context->xut_month  <    1) || (xtm_context->xut_month > 12) ||
        (xtm_context->xut_day    <    1) || (xtm_context->xut_day   > 31) ||
        (xtm_context->xut_hour   >   23) ||
        (xtm_context->xut_minute >   59) ||
        (xtm_context->xut_second >   59) ||
        (xtm_context->xut_msec   >  999))
    {
        return xut_time;
    }
#endif

#ifdef _WIN32
    ULARGE_INTEGER xtime_value;
    FILETIME       xtime_sysfile;
    FILETIME       xtime_locfile;
    SYSTEMTIME     xtime_system;

    xtime_system.wYear         = xtm_context->xut_year  ;
    xtime_system.wMonth        = xtm_context->xut_month ;
    xtime_system.wDay          = xtm_context->xut_day   ;
    xtime_system.wDayOfWeek    = xtm_context->xut_week  ;
    xtime_system.wHour         = xtm_context->xut_hour  ;
    xtime_system.wMinute       = xtm_context->xut_minute;
    xtime_system.wSecond       = xtm_context->xut_second;
    xtime_system.wMilliseconds = xtm_context->xut_msec  ;

    if (SystemTimeToFileTime(&xtime_system, &xtime_locfile))
    {
        if (LocalFileTimeToFileTime(&xtime_locfile, &xtime_sysfile))
        {
            xtime_value.LowPart  = xtime_sysfile.dwLowDateTime ;
            xtime_value.HighPart = xtime_sysfile.dwHighDateTime;
            xut_time = xtime_value.QuadPart - NS100_1970;
        }
    }
#else // !_WIN32
    struct tm       xtm_system;
    struct timeval xtm_value;

    xtm_system.tm_sec   = xtm_context->xut_second;
    xtm_system.tm_min   = xtm_context->xut_minute;
    xtm_system.tm_hour  = xtm_context->xut_hour  ;
    xtm_system.tm_mday  = xtm_context->xut_day   ;
    xtm_system.tm_mon   = xtm_context->xut_month - 1   ;
    xtm_system.tm_year  = xtm_context->xut_year  - 1900;
    xtm_system.tm_wday  = 0;
    xtm_system.tm_yday  = 0;
    xtm_system.tm_isdst = 0;

    xtm_value.tv_sec  = mktime(&xtm_system);
    xtm_value.tv_usec = xtm_context->xut_msec * 1000;
    if (-1 != xtm_value.tv_sec)
    {
        xut_time = ntp_timeval_ns100(&xtm_value);
    }
#endif // _WIN32

    return xut_time;
}

bool ntp_tmctxt_bv(uint64_t xut_time, x_ntp_time_context_t * xtm_context)
{
#ifdef _WIN32
    ULARGE_INTEGER xtime_value;
    FILETIME       xtime_sysfile;
    FILETIME       xtime_locfile;
    SYSTEMTIME     xtime_system;

    if (nullptr == xtm_context)
    {
        return false;
    }

    xtime_value.QuadPart = xut_time + NS100_1970;
    xtime_sysfile.dwLowDateTime  = xtime_value.LowPart;
    xtime_sysfile.dwHighDateTime = xtime_value.HighPart;
    if (!FileTimeToLocalFileTime(&xtime_sysfile, &xtime_locfile))
    {
        return false;
    }

    if (!FileTimeToSystemTime(&xtime_locfile, &xtime_system))
    {
        return false;
    }

    xtm_context->xut_year   = xtime_system.wYear        ;
    xtm_context->xut_month  = xtime_system.wMonth       ;
    xtm_context->xut_day    = xtime_system.wDay         ;
    xtm_context->xut_week   = xtime_system.wDayOfWeek   ;
    xtm_context->xut_hour   = xtime_system.wHour        ;
    xtm_context->xut_minute = xtime_system.wMinute      ;
    xtm_context->xut_second = xtime_system.wSecond      ;
    xtm_context->xut_msec   = xtime_system.wMilliseconds;
#else // !_WIN32
    struct tm xtm_system;
    time_t xtm_time = (time_t)(xut_time / 10000000ULL);
    localtime_r(&xtm_time, &xtm_system);

    xtm_context->xut_year   = xtm_system.tm_year + 1900;
    xtm_context->xut_month  = xtm_system.tm_mon  + 1   ;
    xtm_context->xut_day    = xtm_system.tm_mday       ;
    xtm_context->xut_week   = xtm_system.tm_wday       ;
    xtm_context->xut_hour   = xtm_system.tm_hour       ;
    xtm_context->xut_minute = xtm_system.tm_min        ;
    xtm_context->xut_second = xtm_system.tm_sec        ;
    xtm_context->xut_msec   = (uint32_t)((xut_time % 10000000ULL) / 10000L);
#endif // _WIN32

    return true;
}

bool ntp_tmctxt_tv(const struct timeval *xtm_value, x_ntp_time_context_t * xtm_context)
{
    return ntp_tmctxt_bv(ntp_timeval_ns100(xtm_value), xtm_context);
}

bool ntp_tmctxt_ts(const x_ntp_timestamp_t * const xtm_timestamp, x_ntp_time_context_t * xtm_context)
{
    return ntp_tmctxt_bv(ntp_timestamp_ns100(xtm_timestamp), xtm_context);
}

////////////////////////////////////////////////////////////////////////////////

static bool ntp_ipv4_valid(const char *xszt_vptr, uint32_t *xut_value)
{
    uint8_t xct_ipv[4] = { 0, 0, 0, 0 };

    int32_t xit_itv = 0;
    int32_t xit_sum = 0;
    bool  xbt_okv = false;

    int8_t    xct_iter = '\0';
    const char *xct_iptr = xszt_vptr;

    if (nullptr == xszt_vptr)
    {
        return false;
    }

    for (xct_iter = *xszt_vptr; true; xct_iter = *(++xct_iptr))
    {
        if ((xct_iter != '\0') && (xct_iter >= '0') && (xct_iter <= '9'))
        {
            xit_sum = 10 * xit_sum + (xct_iter - '0');
            xbt_okv = true;
        }
        else if (xbt_okv && (('\0' == xct_iter) || ('.' == xct_iter)) && (xit_itv < (int32_t)sizeof(xct_ipv)) && (xit_sum <= 0xFF))
        {
            xct_ipv[xit_itv++] = xit_sum;
            xit_sum = 0;
            xbt_okv = false;
        }
        else
            break;

        if ('\0' == xct_iter)
        {
            break;
        }
    }

#define MAKE_IPV4_VALUE(b1,b2,b3,b4)  ((uint32_t)(((uint32_t)(b1)<<24)+((uint32_t)(b2)<<16)+((uint32_t)(b3)<<8)+((uint32_t)(b4))))

    xbt_okv = (xit_itv == sizeof(xct_ipv)) ? true : false;
    if (nullptr != xut_value)
    {
        *xut_value = xbt_okv ? MAKE_IPV4_VALUE(xct_ipv[0], xct_ipv[1], xct_ipv[2], xct_ipv[3]) : 0xFFFFFFFF;
    }

#undef MAKE_IPV4_VALUE

    return xbt_okv;
}

static int32_t ntp_gethostbyname(const char *xszt_dname, int32_t xit_family, std::vector< std::string >& xvec_host)
{
    int32_t xit_err = -1;

    struct addrinfo   xai_hint;
    struct addrinfo * xai_rptr = nullptr;
    struct addrinfo * xai_iptr = nullptr;

    char xszt_iphost[TEXT_LEN_256] = { 0 };

    do
    {
        if (nullptr == xszt_dname)
        {
            break;
        }

        memset(&xai_hint, 0, sizeof(xai_hint));
        xai_hint.ai_family   = xit_family;
        xai_hint.ai_socktype = SOCK_DGRAM;

        xit_err = getaddrinfo(xszt_dname, nullptr, &xai_hint, &xai_rptr);
        if (0 != xit_err)
        {
            break;
        }

        for (xai_iptr = xai_rptr; nullptr != xai_iptr; xai_iptr = xai_iptr->ai_next)
        {
            if (xit_family != xai_iptr->ai_family)
            {
                continue;
            }

            memset(xszt_iphost, 0, TEXT_LEN_256);
            if (nullptr == inet_ntop(xit_family, &(((struct sockaddr_in *)(xai_iptr->ai_addr))->sin_addr), xszt_iphost, TEXT_LEN_256))
            {
                continue;
            }

            xvec_host.push_back(std::string(xszt_iphost));
        }

        xit_err = (xvec_host.size() > 0) ? 0 : -3;
    } while (0);

    if (nullptr != xai_rptr)
    {
        freeaddrinfo(xai_rptr);
        xai_rptr = nullptr;
    }

    return xit_err;
}


static int32_t ntp_sockfd_lasterror()
{
#ifdef _WIN32
    return (int32_t)WSAGetLastError();
#else // !_WIN32
    return errno;
#endif // _WIN32
}


static int32_t ntp_sockfd_close(x_sockfd_t xfdt_sockfd)
{
#ifdef _WIN32
    return closesocket(xfdt_sockfd);
#else // !_WIN32
    return close(xfdt_sockfd);
#endif // _WIN32
}


static void ntp_init_request_packet(x_ntp_packet_t * xnpt_dptr)
{
    const uint8_t xct_leap_indicator = 0;
    const uint8_t xct_ntp_version    = 3;
    const uint8_t xct_ntp_mode       = ntp_mode_client;

    xnpt_dptr->xct_li_ver_mode = (xct_leap_indicator << 6) | (xct_ntp_version << 3) | (xct_ntp_mode << 0);
    xnpt_dptr->xct_stratum     = 0;
    xnpt_dptr->xct_poll        = 4;
    xnpt_dptr->xct_percision   = ((-6) & 0xFF);

    xnpt_dptr->xut_root_delay      = (1 << 16);
    xnpt_dptr->xut_root_dispersion = (1 << 16);
    xnpt_dptr->xut_ref_indentifier = 0;

    xnpt_dptr->xtmst_reference.xut_seconds  = 0;
    xnpt_dptr->xtmst_reference.xut_fraction = 0;
    xnpt_dptr->xtmst_originate.xut_seconds  = 0;
    xnpt_dptr->xtmst_originate.xut_fraction = 0;
    xnpt_dptr->xtmst_receive  .xut_seconds  = 0;
    xnpt_dptr->xtmst_receive  .xut_fraction = 0;
    xnpt_dptr->xtmst_transmit .xut_seconds  = 0;
    xnpt_dptr->xtmst_transmit .xut_fraction = 0;
}

static void ntp_ntoh_packet(x_ntp_packet_t * xnpt_nptr)
{
#if 0
    xnpt_nptr->xct_li_ver_mode = xnpt_nptr->xct_li_ver_mode;
    xnpt_nptr->xct_stratum     = xnpt_nptr->xct_stratum    ;
    xnpt_nptr->xct_poll        = xnpt_nptr->xct_poll       ;
    xnpt_nptr->xct_percision   = xnpt_nptr->xct_percision  ;
#endif
    xnpt_nptr->xut_root_delay               = ntohl(xnpt_nptr->xut_root_delay              );
    xnpt_nptr->xut_root_dispersion          = ntohl(xnpt_nptr->xut_root_dispersion         );
    xnpt_nptr->xut_ref_indentifier          = ntohl(xnpt_nptr->xut_ref_indentifier         );
    xnpt_nptr->xtmst_reference.xut_seconds  = ntohl(xnpt_nptr->xtmst_reference.xut_seconds );
    xnpt_nptr->xtmst_reference.xut_fraction = ntohl(xnpt_nptr->xtmst_reference.xut_fraction);
    xnpt_nptr->xtmst_originate.xut_seconds  = ntohl(xnpt_nptr->xtmst_originate.xut_seconds );
    xnpt_nptr->xtmst_originate.xut_fraction = ntohl(xnpt_nptr->xtmst_originate.xut_fraction);
    xnpt_nptr->xtmst_receive  .xut_seconds  = ntohl(xnpt_nptr->xtmst_receive  .xut_seconds );
    xnpt_nptr->xtmst_receive  .xut_fraction = ntohl(xnpt_nptr->xtmst_receive  .xut_fraction);
    xnpt_nptr->xtmst_transmit .xut_seconds  = ntohl(xnpt_nptr->xtmst_transmit .xut_seconds );
    xnpt_nptr->xtmst_transmit .xut_fraction = ntohl(xnpt_nptr->xtmst_transmit .xut_fraction);
}

/**********************************************************/
/**
 * @brief host to network for ntp packet
 */
static void ntp_hton_packet(x_ntp_packet_t * xnpt_nptr)
{
#if 0
    xnpt_nptr->xct_li_ver_mode = xnpt_nptr->xct_li_ver_mode;
    xnpt_nptr->xct_stratum     = xnpt_nptr->xct_stratum    ;
    xnpt_nptr->xct_poll        = xnpt_nptr->xct_poll       ;
    xnpt_nptr->xct_percision   = xnpt_nptr->xct_percision  ;
#endif
    xnpt_nptr->xut_root_delay               = htonl(xnpt_nptr->xut_root_delay              );
    xnpt_nptr->xut_root_dispersion          = htonl(xnpt_nptr->xut_root_dispersion         );
    xnpt_nptr->xut_ref_indentifier          = htonl(xnpt_nptr->xut_ref_indentifier         );
    xnpt_nptr->xtmst_reference.xut_seconds  = htonl(xnpt_nptr->xtmst_reference.xut_seconds );
    xnpt_nptr->xtmst_reference.xut_fraction = htonl(xnpt_nptr->xtmst_reference.xut_fraction);
    xnpt_nptr->xtmst_originate.xut_seconds  = htonl(xnpt_nptr->xtmst_originate.xut_seconds );
    xnpt_nptr->xtmst_originate.xut_fraction = htonl(xnpt_nptr->xtmst_originate.xut_fraction);
    xnpt_nptr->xtmst_receive  .xut_seconds  = htonl(xnpt_nptr->xtmst_receive  .xut_seconds );
    xnpt_nptr->xtmst_receive  .xut_fraction = htonl(xnpt_nptr->xtmst_receive  .xut_fraction);
    xnpt_nptr->xtmst_transmit .xut_seconds  = htonl(xnpt_nptr->xtmst_transmit .xut_seconds );
    xnpt_nptr->xtmst_transmit .xut_fraction = htonl(xnpt_nptr->xtmst_transmit .xut_fraction);
}

/**********************************************************/
/**
 * @brief send request to NTP server and get T1 T2 T3 T4
 * <pre>
 *     1. client send request to NTP server with client's local time T1
 *     2. when NTP server receives client's request add server's localtime T2
 *     3. when NTP response leaves server, stamping with server's current time T3
 *     4. when client receives response, client stamps its current time T4
 * </pre>
 *
 * @param [in ] xszt_host : NTP ip address
 * @param [in ] xut_port  : NTP port default 123
 * @param [in ] xut_tmout : timeout in milliseconds
 * @param [out] xit_tmlst : on success returns (T1 T2 T3 T4)
 *
 * @return int32_t 0 on success, error code on failure
 *
 */
static int32_t ntp_get_time_values(const char *xszt_host, uint16_t xut_port, uint32_t xut_tmout, int64_t xit_tmlst[4])
{
    int32_t xit_err = -1;

    x_sockfd_t      xfdt_sockfd = X_INVALID_SOCKFD;
    x_ntp_packet_t  xnpt_buffer;
    struct timeval xtm_value;

    int32_t xit_addrlen = sizeof(struct sockaddr_in);
    struct sockaddr_in skaddr_host;

    do
    {
        if ((nullptr == xszt_host) || (xut_tmout <= 0) || (nullptr == xit_tmlst))
        {
            break;
        }

        xfdt_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (X_INVALID_SOCKFD == xfdt_sockfd)
        {
            break;
        }

#ifdef _WIN32
        setsockopt(xfdt_sockfd, SOL_SOCKET, SO_SNDTIMEO, (x_char_t *)&xut_tmout, sizeof(uint32_t));
        setsockopt(xfdt_sockfd, SOL_SOCKET, SO_RCVTIMEO, (x_char_t *)&xut_tmout, sizeof(uint32_t));
#else // !_WIN32
        xtm_value.tv_sec  = (long)((xut_tmout / 1000));
        xtm_value.tv_usec = (long)((xut_tmout % 1000) * 1000);
        setsockopt(xfdt_sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&xtm_value, sizeof(struct timeval));
        setsockopt(xfdt_sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&xtm_value, sizeof(struct timeval));
#endif // _WIN32


        memset(&skaddr_host, 0, sizeof(struct sockaddr_in));
        skaddr_host.sin_family = AF_INET;
        skaddr_host.sin_port   = htons(xut_port);
        inet_pton(AF_INET, xszt_host, &skaddr_host.sin_addr.s_addr);

        ntp_init_request_packet(&xnpt_buffer);

        // sender's localtime when request is sending out
        ntp_gettimeofday(&xtm_value);
        ntp_timeval_to_timestamp(&xnpt_buffer.xtmst_originate, &xtm_value);

        // T1
        xit_tmlst[0] = (int64_t)ntp_timeval_ns100(&xtm_value);

        ntp_hton_packet(&xnpt_buffer);

        xit_err = sendto(xfdt_sockfd,
                         (char *)&xnpt_buffer,
                         sizeof(x_ntp_packet_t),
                         0,
                         (sockaddr *)&skaddr_host,
                         sizeof(struct sockaddr_in));
        if (xit_err < 0)
        {
            xit_err = ntp_sockfd_lasterror();
            continue;
        }

        memset(&xnpt_buffer, 0, sizeof(x_ntp_packet_t));

        xit_err = recvfrom(xfdt_sockfd,
                           (char *)&xnpt_buffer,
                           sizeof(x_ntp_packet_t),
                           0,
                           (sockaddr *)&skaddr_host,
                           (socklen_t *)&xit_addrlen);
        if (xit_err < 0)
        {
            xit_err = ntp_sockfd_lasterror();
            break;
        }

        if (sizeof(x_ntp_packet_t) != xit_err)
        {
            xit_err = -1;
            break;
        }

        // T4
        xit_tmlst[3] = (int64_t)ntp_gettimevalue();

        ntp_ntoh_packet(&xnpt_buffer);

        xit_tmlst[1] = (int64_t)ntp_timestamp_ns100(&xnpt_buffer.xtmst_receive ); // T2
        xit_tmlst[2] = (int64_t)ntp_timestamp_ns100(&xnpt_buffer.xtmst_transmit); // T3

        xit_err = 0;
    } while (0);

    if (X_INVALID_SOCKFD != xfdt_sockfd)
    {
        ntp_sockfd_close(xfdt_sockfd);
        xfdt_sockfd = X_INVALID_SOCKFD;
    }

    return xit_err;
}

/**********************************************************/
/**
 * @brief send request to NTP server and get server's timestamp
 *
 * @param [in ] xszt_host : NTP server's ip address
 * @param [in ] xut_port  : NTP port default 123
 * @param [in ] xut_tmout : timeout in milliseconds
 * @param [out] xut_timev : 100ns time returned on success
 *
 * @return int32_t 0 on success, error code on failure
 *
 */
int32_t ntp_get_time(const char *xszt_host, uint16_t xut_port, uint32_t xut_tmout, uint64_t *xut_timev)
{
    int32_t xit_err = -1;
    std::vector< std::string > xvec_host;

    int64_t xit_tmlst[4] = { 0 };

    if ((nullptr == xszt_host) || (xut_tmout <= 0) || (nullptr == xut_timev))
    {
        return -1;
    }

    // get ip addresses list
    if (ntp_ipv4_valid(xszt_host, nullptr))
    {
        xvec_host.push_back(std::string(xszt_host));
    }
    else
    {
        xit_err = ntp_gethostbyname(xszt_host, AF_INET, xvec_host);
        if (0 != xit_err)
        {
            return xit_err;
        }
    }

    if (xvec_host.empty())
    {
        return -1;
    }

    for (auto& ntp_host : xvec_host)
    {
        xdbg("========================================");
        xdbg("  %s -> %s", xszt_host, ntp_host.c_str());

        xit_err = ntp_get_time_values(ntp_host.c_str(), xut_port, xut_tmout, xit_tmlst);
        if (0 == xit_err)
        {
            // T = T4 + ((T2 - T1) + (T3 - T4)) / 2;
            *xut_timev = xit_tmlst[3] + ((xit_tmlst[1] - xit_tmlst[0]) + (xit_tmlst[2] - xit_tmlst[3])) / 2;

            bv_output("time1", xit_tmlst[0]);
            bv_output("time2", xit_tmlst[1]);
            bv_output("time3", xit_tmlst[2]);
            bv_output("time4", xit_tmlst[3]);
            bv_output("timev", *xut_timev);
            bv_output("timec", ntp_gettimevalue());

            break;
        }
    }

    return xit_err;
}

} // end of namespace top
