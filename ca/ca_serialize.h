#ifndef __CA_SERIALIZE_H__
#define __CA_SERIALIZE_H__

#include <stdint.h>
#include <stdbool.h>
#include "ca_buffer.h"
#include "ca_cstr.h"

#ifdef __cplusplus
extern "C" {
#endif

bool deser_u32(uint32_t *vo, struct const_buffer *buf);
bool deser_bytes(void *po, struct const_buffer *buf, size_t len);
bool deser_varlen(uint32_t *lo, struct const_buffer *buf);
bool deser_u16(uint16_t *vo, struct const_buffer *buf);
bool deser_u64(uint64_t *vo, struct const_buffer *buf);
bool deser_varstr(cstring **so, struct const_buffer *buf);

void ser_bytes(cstring *s, const void *p, size_t len);
void ser_u32(cstring *s, uint32_t v_);
void ser_varlen(cstring *s, uint32_t vlen);
void ser_u16(cstring *s, uint16_t v_);
void ser_varstr(cstring *s, cstring *s_in);

#ifdef __cplusplus
}
#endif

#endif
