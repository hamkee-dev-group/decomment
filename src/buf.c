#include "buf.h"
#include <stdlib.h>
#include <string.h>

#define DC_BUF_INIT_CAP 4096

void dc_buf_init(dc_buf_t *b)
{
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

void dc_buf_free(dc_buf_t *b)
{
    free(b->data);
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

dc_result_t dc_buf_ensure(dc_buf_t *b, size_t additional)
{
    size_t needed = b->len + additional;
    if (needed < b->len)
        return DC_ERR_ALLOC;
    if (needed <= b->cap)
        return DC_OK;

    size_t newcap = b->cap ? b->cap : DC_BUF_INIT_CAP;
    while (newcap < needed)
    {
        size_t doubled = newcap * 2;
        if (doubled <= newcap)
            return DC_ERR_ALLOC; 
        newcap = doubled;
    }

    char *p = realloc(b->data, newcap);
    if (!p)
        return DC_ERR_ALLOC;
    b->data = p;
    b->cap = newcap;
    return DC_OK;
}

dc_result_t dc_buf_push(dc_buf_t *b, char c)
{
    dc_result_t r = dc_buf_ensure(b, 1);
    if (r != DC_OK)
        return r;
    b->data[b->len++] = c;
    return DC_OK;
}

dc_result_t dc_buf_append(dc_buf_t *b, const char *data, size_t len)
{
    if (len == 0)
        return DC_OK;
    dc_result_t r = dc_buf_ensure(b, len);
    if (r != DC_OK)
        return r;
    memcpy(b->data + b->len, data, len);
    b->len += len;
    return DC_OK;
}

dc_result_t dc_buf_append_str(dc_buf_t *b, const char *str)
{
    return dc_buf_append(b, str, strlen(str));
}

void dc_buf_clear(dc_buf_t *b)
{
    b->len = 0;
}

char *dc_buf_detach(dc_buf_t *b, size_t *out_len)
{
    char *data = b->data;
    if (out_len)
        *out_len = b->len;
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
    return data;
}
