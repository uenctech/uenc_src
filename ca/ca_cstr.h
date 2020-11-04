#ifndef __CA_CSTR_H__
#define __CA_CSTR_H__
#include <stdbool.h>                    
#include <stddef.h>                     
#include <sys/types.h>                  

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cstring {
    char    *str;       
    size_t  len;        
    size_t  alloc;      
} cstring;

void cstr_free(cstring *s, bool free_buf);
cstring *cstr_new_sz(size_t sz);
bool cstr_append_buf(cstring *s, const void *buf, size_t sz);
bool cstr_alloc_min_sz(cstring *s, size_t sz);
cstring *cstr_new(const char *init_str);
cstring *cstr_new_buf(const void *buf, size_t sz);
bool cstr_resize(cstring *s, size_t sz);
bool cstr_equal(const cstring *a, const cstring *b);

#ifdef __cplusplus
}
#endif

#endif
