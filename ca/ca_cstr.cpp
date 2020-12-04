#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ca_cstr.h"

bool cstr_resize(cstring *s, size_t new_sz)
{
    // no change
    if (new_sz == s->len)
        return true;

    // truncate string
    if (new_sz <= s->len) {
        s->len = new_sz;
        s->str[s->len] = 0;
        return true;
    }

    // increase string size
    if (!cstr_alloc_min_sz(s, new_sz))
        return false;

    // contents of string tail undefined
    
    s->len = new_sz;
    s->str[s->len] = 0;

    return true;
}

void cstr_free(cstring *s, bool free_buf)
{       
    if (!s)
        return;
    
    if (free_buf)
        free(s->str);
    
    memset(s, 0, sizeof(*s));
    free(s);
}
cstring *cstr_new_sz(size_t sz)
{   
    cstring *s = (cstring *)calloc(1, sizeof(cstring));
    if (!s)
        return NULL;
    
    if (!cstr_alloc_min_sz(s, sz)) {
        free(s);
        return NULL;
    }

    return s;
}
bool cstr_append_buf(cstring *s, const void *buf, size_t sz)
{   
    if (!cstr_alloc_min_sz(s, s->len + sz))
        return false;
    
    memcpy(s->str + s->len, buf, sz);
    s->len += sz;
    s->str[s->len] = 0;
    
    return true;
}
bool cstr_alloc_min_sz(cstring *s, size_t sz)
{
    sz++;       // NUL overhead

    if (s->alloc && (s->alloc >= sz))
        return true;

    unsigned int shift = 3;
    unsigned int al_sz;
    while ((al_sz = (1 << shift)) < sz)
        shift++;

    char *new_s = (char *)realloc(s->str, al_sz);
    if (!new_s)
        return false;

    s->str = new_s;
    s->alloc = al_sz;
    s->str[s->len] = 0;

    return true;
}

cstring *cstr_new(const char *init_str)
{
    if (!init_str || !*init_str)
        return cstr_new_sz(0);

    size_t slen = strlen(init_str);
    return cstr_new_buf(init_str, slen);
}

cstring *cstr_new_buf(const void *buf, size_t sz)
{
    cstring *s = cstr_new_sz(sz);
    if (!s)
        return NULL;

    memcpy(s->str, buf, sz);
    s->len = sz;
    s->str[s->len] = 0;

    return s;
}

bool cstr_equal(const cstring *a, const cstring *b)
{
    if (a == b)
        return true;
    if (!a || !b)
        return false;
    if (a->len != b->len)
        return false;
    if (a->len == 0)
        return true;
    return (memcmp(a->str, b->str, a->len) == 0);
}
