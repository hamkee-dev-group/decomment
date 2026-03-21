#ifndef DECOMMENT_H
#define DECOMMENT_H

#define DC_VERSION_MAJOR 0
#define DC_VERSION_MINOR 1
#define DC_VERSION_PATCH 0
#define DC_VERSION_STRING "0.1.0"

 
#define DC_EXIT_SUCCESS      0
#define DC_EXIT_ERROR        1
#define DC_EXIT_SKIPPED      2
#define DC_EXIT_CHECK_DIFF   3
#define DC_EXIT_USAGE        4

 
typedef enum {
    DC_OK = 0,
    DC_ERR_IO,
    DC_ERR_ALLOC,
    DC_ERR_BINARY,
    DC_ERR_UNSUPPORTED,
    DC_ERR_ENCODING,
    DC_ERR_PERMISSION,
    DC_ERR_TOOLARGE,
    DC_ERR_SPECIAL,
    DC_ERR_SYMLOOP,
    DC_ERR_INTERNAL,
    DC_ERR_USAGE
} dc_result_t;

 
typedef enum {
    DC_ACTION_MODIFIED,
    DC_ACTION_UNCHANGED,
    DC_ACTION_SKIPPED,
    DC_ACTION_ERROR
} dc_action_t;

 
#define DC_MAX_FILE_SIZE (256u * 1024u * 1024u)

#endif  
