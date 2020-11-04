#ifndef __CA_BUFFER_H__
#define __CA_BUFFER_H__

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct buffer {
    void        *p;
    size_t      len;
};      
        
struct const_buffer {
    const void  *p;
    size_t      len;
};

#ifdef __cplusplus
}           
#endif      
            
#endif
