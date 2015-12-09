#ifndef __FBACKUP_ERROR_H__
#define __FBACKUP_ERROR_H__

#define FB_E_BASE                       (0xD0000000)
#define FB_E_NO_MEMORY                  (FB_E_BASE + 1)
#define FB_E_UNK_REQUEST                (FB_E_BASE + 2)
#define FB_E_INVAL                      (FB_E_BASE + 3)
#define FB_E_UNIMPL                     (FB_E_BASE + 4)
#define FB_E_BAD_MAGIC                  (FB_E_BASE + 5)
#define FB_E_BAD_SIZE                   (FB_E_BASE + 6)
#define FB_E_INCOMP_HEADER              (FB_E_BASE + 7)
#define FB_E_INCOMP_DATA                (FB_E_BASE + 8)

#endif