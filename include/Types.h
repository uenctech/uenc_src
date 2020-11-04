
























#ifndef _EBPC_TYPES_H_
#define _EBPC_TYPES_H_

#ifndef USE_STD_STRING
#include <wx/string.h>		
#endif 

#include <list>			
#include <vector>		

#ifndef _MSC_VER
	#ifndef __STDC_FORMAT_MACROS
		#define __STDC_FORMAT_MACROS
	#endif
	#include <inttypes.h>
	#define LONGLONG(x) x##ll
	#define ULONGLONG(x) x##llu
#else
	typedef unsigned __int8 byte;
	typedef unsigned __int8 uint8_t;
	typedef unsigned __int16 uint16_t;
	typedef unsigned __int32 uint32_t;
	typedef unsigned __int64 uint64_t;
	typedef signed __int8 int8_t;
	typedef signed __int16 int16_t;
	typedef signed __int32 int32_t;
	typedef signed __int64 int64_t;
	#define LONGLONG(x) x##i64
	#define ULONGLONG(x) x##ui64
#endif






































/* 
 * Backwards compatibility with emule.
 * Note that the int* types are indeed unsigned.
 */
typedef uint8_t		int8;
typedef uint8_t		uint8;
typedef uint16_t	int16;
typedef uint16_t	uint16;
typedef uint32_t	int32;
typedef uint32_t	uint32;
typedef uint64_t	int64;
typedef uint64_t	uint64;
typedef int8_t		sint8;
typedef int16_t		sint16;
typedef int32_t		sint32;
typedef int64_t		sint64;
typedef uint8_t		byte;


class CKnownFile;



#ifndef USE_STD_STRING
typedef std::list<wxString> CStringList;
#endif
typedef std::list<CKnownFile*> CKnownFilePtrList;


typedef std::vector<uint8>  ArrayOfUInts8;
typedef std::vector<uint16> ArrayOfUInts16;
typedef std::vector<uint32> ArrayOfUInts32;
typedef std::vector<uint64> ArrayOfUInts64;

typedef std::list<uint32>	ListOfUInts32;







#ifndef USE_STD_STRING
static const wxString EmptyString = wxEmptyString;
#endif

#ifndef __cplusplus
	typedef int bool;
#endif


#ifdef _WIN32			


#if 0

#ifdef _MSC_VER
	#define NOMINMAX
	#include <windows.h> 
#else
	#include <windef.h>	
	#include <wingdi.h>	
	#include <winuser.h>	
	#include <winbase.h> 
#endif

#else

	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
	#include <windows.h> 

#endif


	
	#ifndef W_OK
		enum
		{
			F_OK = 0,   
			X_OK = 1,   
			W_OK = 2,   
			R_OK = 4    
		};
	#endif 
	#ifdef __WXMSW__
		#include <wx/msw/winundef.h>	
	#endif
	#undef GetUserName
#else 
	typedef struct sRECT {
	  uint32 left;
	  uint32 top;
	  uint32 right;
	  uint32 bottom;
	} RECT;
#endif 


#endif 

