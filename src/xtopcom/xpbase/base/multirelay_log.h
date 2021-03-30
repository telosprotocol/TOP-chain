#pragma once

#include "xpbase/base/top_log.h"

// log for smaug
 /*
#define SMDEBUG(xrequest_id, seq, fmt, ...) TOP_DEBUG("<smaug>MultiRelay <" xrequest_id "> <" seq ">" fmt, ## __VA_ARGS__)
#define SMINFO(xrequest_id, seq, fmt, ...) TOP_DEBUG("<smaug>MultiRelay <" xrequest_id "> <" seq ">" fmt, ## __VA_ARGS__)
#define SMWARN(xrequest_id, seq, fmt, ...) TOP_DEBUG("<smaug>MultiRelay <" xrequest_id "> <" seq ">" fmt, ## __VA_ARGS__)
#define SMERROR(xrequest_id, seq, fmt, ...) TOP_DEBUG("<smaug>MultiRelay <" xrequest_id "> <" seq ">" fmt, ## __VA_ARGS__)
*/

#define SMERROR(xrequestid, seq, fmt, ...) TOP_ERROR("<smaug>MultiRelay xrequestid<%s> seq<%d>  " fmt, xrequestid, seq, ## __VA_ARGS__)
#define SMDEBUG(xrequestid, seq, fmt, ...) TOP_DEBUG("<smaug>MultiRelay xrequestid<%s> seq<%d>  " fmt, xrequestid, seq, ## __VA_ARGS__)
#define SMINFO(xrequestid, seq, fmt, ...)  TOP_INFO("<smaug>MultiRelay xrequestid<%s> seq<%d>  " fmt, xrequestid, seq, ## __VA_ARGS__)
#define SMWARN(xrequestid, seq, fmt, ...)  TOP_WARN("<smaug>MultiRelay xrequestid<%s> seq<%d>  " fmt, xrequestid, seq, ## __VA_ARGS__)
