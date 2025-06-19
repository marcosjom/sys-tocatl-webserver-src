//
//  TWStr.h
//  tocatl-webserver
//
//  Created by Marcos Ortega on 26/11/21.
//

#ifndef TWStrTemplate_h
#define TWStrTemplate_h

#include "nb/NBFrameworkDefs.h"
#include "nb/core/NBMemory.h"
#include "nb/core/NBString.h"

//DECLARATION (used in .h files)

#ifdef __cplusplus
#	define TW_DB_STR_CLASS_DEC(TYPE, INLINE_BUFF_LEN)	\
		extern "C" { \
			\
			typedef struct ST ## TYPE ## _ { \
				UI32		len; /*value len*/ \
				char		inln[INLINE_BUFF_LEN]; /*inline value (if fits inside)*/ \
				char*		heap;	/*allocated pointer (if doest not fir inlined)*/ \
				UI32		heapSz;	/*amount allocated (if doest not fir inlined)*/ \
			} ST ## TYPE; \
			\
			void TYPE ## _init(ST ## TYPE* obj); \
			void TYPE ## _release(ST ## TYPE* obj); \
			void TYPE ## _resignToPayload(ST ## TYPE* obj); \
			\
			const char* TYPE ## _get(const ST ## TYPE* obj); \
			void TYPE ## _setBytes(ST ## TYPE* obj, const char* val, const UI32 valSz); \
			\
			BOOL NBCompare_ ## TYPE(const ENCompareMode mode, const void* data1, const void* data2, const UI32 dataSz); \
			BOOL NBCompare_ ## TYPE ## Ptr(const ENCompareMode mode, const void* data1, const void* data2, const UI32 dataSz); \
		}
#else
#	define TW_DB_STR_CLASS_DEC(TYPE, INLINE_BUFF_LEN)	\
		\
		typedef struct ST ## TYPE ## _ { \
			UI32		len; /*value len*/ \
			char		inln[INLINE_BUFF_LEN]; /*inline value (if fits inside)*/ \
			char*		heap;	/*allocated pointer (if doest not fir inlined)*/ \
			UI32		heapSz;	/*amount allocated (if doest not fir inlined)*/ \
		} ST ## TYPE; \
		\
		void TYPE ## _init(ST ## TYPE* obj); \
		void TYPE ## _release(ST ## TYPE* obj); \
		void TYPE ## _resignToPayload(ST ## TYPE* obj); \
		\
		const char* TYPE ## _get(const ST ## TYPE* obj); \
		void TYPE ## _setBytes(ST ## TYPE* obj, const char* val, const UI32 valSz); \
		\
		BOOL NBCompare_ ## TYPE(const ENCompareMode mode, const void* data1, const void* data2, const UI32 dataSz); \
		BOOL NBCompare_ ## TYPE ## Ptr(const ENCompareMode mode, const void* data1, const void* data2, const UI32 dataSz);
#endif

//DEFINITION (used in .c files)

#define TW_DB_STR_CLASS_DEF(TYPE)	\
	\
	void TYPE ## _init(ST ## TYPE* obj){ \
		NBMemory_setZeroSt(*obj, ST ## TYPE); \
	} \
	\
	void TYPE ## _release(ST ## TYPE* obj){ \
		if(obj->heap != NULL){ \
			NBMemory_free(obj->heap); \
			obj->heap = NULL; \
			obj->heapSz = 0; \
		} \
	} \
	\
	void TYPE ## _resignToPayload(ST ## TYPE* obj){ \
		obj->heap	= NULL; \
		obj->heapSz	= 0; \
		obj->len	= 0; \
	} \
	\
	const char* TYPE ## _get(const ST ## TYPE* obj){ \
		return (obj->heap != NULL ? obj->heap : obj->inln); \
	} \
	\
	void TYPE ## _setBytes(ST ## TYPE* obj, const char* val, const UI32 valSz){ \
		if((valSz + 1) <= sizeof(obj->inln)){ \
			/*copy inline*/ \
			NBMemory_copy(obj->inln, val, valSz); \
			obj->inln[valSz] = '\0'; \
			/*free heap (if allocated)*/ \
			if(obj->heap != NULL){ \
				NBMemory_free(obj->heap); \
				obj->heap = NULL; \
				obj->heapSz = 0; \
			} \
		} else { \
			/*resize heap (if necesary)*/ \
			if(obj->heapSz < (valSz + 1)){ \
				if(obj->heap != NULL){ \
					NBMemory_free(obj->heap); \
				} \
				obj->heapSz = (valSz + 1); \
				obj->heap	= NBMemory_alloc(obj->heapSz); \
			} \
			/*copy to heap*/ \
			NBMemory_copy(obj->heap, val, valSz); \
			obj->heap[valSz] = '\0'; \
		} \
		obj->len = valSz; \
	} \
	\
	BOOL NBCompare_ ## TYPE(const ENCompareMode mode, const void* data1, const void* data2, const UI32 dataSz){ \
		NBASSERT(dataSz == sizeof(ST ## TYPE)) \
		if(dataSz == sizeof(ST ## TYPE)){ \
			ST ## TYPE* o1 = (ST ## TYPE*)data1; \
			ST ## TYPE* o2 = (ST ## TYPE*)data2; \
			switch (mode) { \
				case ENCompareMode_Equal: \
					return (o1->len == o2->len && NBString_strIsEqualBytes((o1->heap != NULL ? o1->heap : o1->inln), o1->len, (o2->heap != NULL ? o2->heap : o2->inln), o2->len)); \
				case ENCompareMode_Lower: \
					return (o1->len < o2->len || (o1->len == o2->len && NBString_strIsLowerBytes((o1->heap != NULL ? o1->heap : o1->inln), o1->len, (o2->heap != NULL ? o2->heap : o2->inln), o2->len))); \
				case ENCompareMode_LowerOrEqual: \
					return (o1->len < o2->len || (o1->len == o2->len && NBString_strIsLowerOrEqualBytes((o1->heap != NULL ? o1->heap : o1->inln), o1->len, (o2->heap != NULL ? o2->heap : o2->inln), o2->len))); \
				case ENCompareMode_Greater: \
					return (o1->len > o2->len || (o1->len == o2->len && NBString_strIsGreaterBytes((o1->heap != NULL ? o1->heap : o1->inln), o1->len, (o2->heap != NULL ? o2->heap : o2->inln), o2->len))); \
				case ENCompareMode_GreaterOrEqual: \
					return (o1->len > o2->len || (o1->len == o2->len && NBString_strIsGreaterOrEqualBytes((o1->heap != NULL ? o1->heap : o1->inln), o1->len, (o2->heap != NULL ? o2->heap : o2->inln), o2->len))); \
				default: \
					NBASSERT(FALSE) \
					break; \
			} \
		} \
		NBASSERT(FALSE) \
		return FALSE; \
	} \
	\
	BOOL NBCompare_ ## TYPE ## Ptr(const ENCompareMode mode, const void* data1, const void* data2, const UI32 dataSz){ \
		NBASSERT(dataSz == sizeof(ST ## TYPE*)) \
		if(dataSz == sizeof(ST ## TYPE*)){ \
			ST ## TYPE* o1 = *((ST ## TYPE**)data1); \
			ST ## TYPE* o2 = *((ST ## TYPE**)data2); \
			switch (mode) { \
				case ENCompareMode_Equal: \
					return (o1->len == o2->len && NBString_strIsEqualBytes((o1->heap != NULL ? o1->heap : o1->inln), o1->len, (o2->heap != NULL ? o2->heap : o2->inln), o2->len)); \
				case ENCompareMode_Lower: \
					return (o1->len < o2->len || (o1->len == o2->len && NBString_strIsLowerBytes((o1->heap != NULL ? o1->heap : o1->inln), o1->len, (o2->heap != NULL ? o2->heap : o2->inln), o2->len))); \
				case ENCompareMode_LowerOrEqual: \
					return (o1->len < o2->len || (o1->len == o2->len && NBString_strIsLowerOrEqualBytes((o1->heap != NULL ? o1->heap : o1->inln), o1->len, (o2->heap != NULL ? o2->heap : o2->inln), o2->len))); \
				case ENCompareMode_Greater: \
					return (o1->len > o2->len || (o1->len == o2->len && NBString_strIsGreaterBytes((o1->heap != NULL ? o1->heap : o1->inln), o1->len, (o2->heap != NULL ? o2->heap : o2->inln), o2->len))); \
				case ENCompareMode_GreaterOrEqual: \
					return (o1->len > o2->len || (o1->len == o2->len && NBString_strIsGreaterOrEqualBytes((o1->heap != NULL ? o1->heap : o1->inln), o1->len, (o2->heap != NULL ? o2->heap : o2->inln), o2->len))); \
				default: \
					NBASSERT(FALSE) \
					break; \
			} \
		} \
		NBASSERT(FALSE) \
		return FALSE; \
	}

#endif /* TWStr_h */
