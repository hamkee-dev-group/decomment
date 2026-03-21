#ifndef DC_LANG_H
#define DC_LANG_H

#include <stddef.h>
#include <stdint.h>
#include "decomment.h"
#include "buf.h"



typedef dc_result_t (*dc_strip_fn)(const char *input, size_t input_len,
                                    dc_buf_t *output, uint32_t flags);
typedef struct {
    const char *name;            
    const char *display_name;    
    const char * const *extensions;  
    const char * const *filenames;   
    dc_strip_fn strip;           
    uint32_t flags;              
    const char *notes;           
} dc_lang_t;

void dc_lang_init(void);
const dc_lang_t *dc_lang_detect(const char *path, const char *force_lang);
const dc_lang_t *dc_lang_detect_shebang(const char *data, size_t len);
const dc_lang_t *dc_lang_find(const char *name);
const dc_lang_t * const *dc_lang_all(void);
int dc_ext_in_list(const char *ext, const char *csv_list);

#endif  
