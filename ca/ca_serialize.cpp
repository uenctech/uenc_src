#include <stdint.h>
#include <string.h>
#include "ca_serialize.h"
#include "ca_buffer.h"
#include "ca_ios.h"

bool deser_bytes(void *po, struct const_buffer *buf, size_t len)
{
    if (buf->len < len)
        return false;

    memcpy(po, buf->p, len);
	char *p =(char *)buf->p;
    p += len;
    buf->p = p;
    buf->len -= len;

    return true;
} 

bool deser_u32(uint32_t *vo, struct const_buffer *buf)
{ 
    uint32_t v; 

    if (!deser_bytes(&v, buf, sizeof(v)))
        return false; 

//    *vo = le32toh(v); 
    *vo = E32toh(v); 
    return true;
}

bool deser_varlen(uint32_t *lo, struct const_buffer *buf)
{   
    uint32_t len;
    
    unsigned char c;
    if (!deser_bytes(&c, buf, 1)) return false;
    
    if (c == 253) { 
        uint16_t v16;
        if (!deser_u16(&v16, buf)) return false;
        len = v16;
    }   
    else if (c == 254) {
        uint32_t v32;
        if (!deser_u32(&v32, buf)) return false;
        len = v32;
    }
    else if (c == 255) {
        uint64_t v64;
        if (!deser_u64(&v64, buf)) return false;
        len = (uint32_t) v64;   /* WARNING: truncate */
    }
    else
        len = c;

    *lo = len;
    return true;
}
bool deser_u16(uint16_t *vo, struct const_buffer *buf)
{
    uint16_t v;

    if (!deser_bytes(&v, buf, sizeof(v)))
        return false;

 //   *vo = le16toh(v);
    *vo = E16toh(v);
    return true;
}
bool deser_u64(uint64_t *vo, struct const_buffer *buf)
{
    uint64_t v;

    if (!deser_bytes(&v, buf, sizeof(v)))
        return false;

//    *vo = le64toh(v);
    *vo = E64toh(v);
    return true;
}
bool deser_varstr(cstring **so, struct const_buffer *buf)
{   
    if (*so) {
        cstr_free(*so, true);
        *so = NULL;
    }
    
    uint32_t len;
    if (!deser_varlen(&len, buf)) return false;
    
    if (buf->len < len)
        return false;

    cstring *s = cstr_new_sz(len);
    cstr_append_buf(s, buf->p, len);

    char *p =(char *)buf->p;
    p += len;
    buf->p = p;
    buf->len -= len;

    *so = s;

    return true;
}

void ser_bytes(cstring *s, const void *p, size_t len)
{   
    cstr_append_buf(s, p, len);
}

void ser_u32(cstring *s, uint32_t v_)
{   
 //   uint32_t v = htole32(v_);
    uint32_t v = H32toh(v_);
    cstr_append_buf(s, &v, sizeof(v));
}
void ser_varlen(cstring *s, uint32_t vlen)
{
    unsigned char c;

    if (vlen < 253) {
        c = vlen;
        ser_bytes(s, &c, 1);
    }

    else if (vlen < 0x10000) {
        c = 253;
        ser_bytes(s, &c, 1);
        ser_u16(s, (uint16_t) vlen);
    }

    else {
        c = 254;
        ser_bytes(s, &c, 1);
        ser_u32(s, vlen);
    }

    /* u64 case intentionally not implemented */
}
void ser_u16(cstring *s, uint16_t v_)
{
  //  uint16_t v = htole16(v_);
    uint16_t v = H16toh(v_);
    cstr_append_buf(s, &v, sizeof(v));
}
void ser_varstr(cstring *s, cstring *s_in)
{   
    if (!s_in || !s_in->len) {
        ser_varlen(s, 0);
        return;
    }

    ser_varlen(s, s_in->len);
    ser_bytes(s, s_in->str, s_in->len);
}
