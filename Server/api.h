#ifndef __FBACKUP_SERVER_API_H__
#define __FBACKUP_SERVER_API_H__

enum {
    FB_SRV_REQ_TYPE_INVALID = 0,
    FB_SRV_REQ_TYPE_TIME = 1,
    FB_SRV_REQ_TYPE_MAX
};

#define FB_SRV_REQ_MAGIC 'qrBF'
#define FB_SRV_RESP_MAGIC 'prBF'

#define FB_SRV_PORT L"9111"
#define FB_SRV_MAX_BODY_SIZE (64*4096)

#pragma pack(push, 1)

typedef struct _FB_SRV_REQ_HEADER {
    unsigned long Magic;
    unsigned long Size;
    unsigned long Type;
    unsigned long Id;
} FB_SRV_REQ_HEADER, *PFB_SRV_REQ_HEADER;

typedef struct _FB_SRV_RESP_HEADER {
    unsigned long Magic;
    unsigned long Size;
    unsigned long Err;
    unsigned long Type;
    unsigned long Id;
} FB_SRV_RESP_HEADER, *PFB_SRV_RESP_HEADER;

typedef struct _FB_SRV_RESP_TIME_BODY {
    unsigned short Year;
    unsigned short Month;
    unsigned short DayOfWeek;
    unsigned short Day;
    unsigned short Hour;
    unsigned short Minute;
    unsigned short Second;
    unsigned short Milliseconds;
} FB_SRV_RESP_TIME_BODY, *PFB_SRV_RESP_TIME_BODY;

typedef struct _FB_SRV_RESP_TIME {
    FB_SRV_RESP_HEADER Header;
    FB_SRV_RESP_TIME_BODY Body;
} FB_SRV_RESP_TIME, *PFB_SRV_RESP_TIME;

#pragma pack(pop)

#endif