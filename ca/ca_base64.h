#ifndef __CA_BASE64__
#define __CA_BASE64__

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <string>
#include <iostream>

unsigned long base64_encode(const unsigned char *text, unsigned long text_len, unsigned char *encode);
unsigned long base64_decode(const unsigned char *code, unsigned long code_len, unsigned char *plain);


#endif
