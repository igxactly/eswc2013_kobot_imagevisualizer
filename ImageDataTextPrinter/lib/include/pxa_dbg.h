/*
 *(C) Copyright 2006 Marvell International Ltd.  
 * All Rights Reserved 
 *
 * Author:     Neo Li
 * Created:    March 07, 2007
 * Copyright:  Marvell Semiconductor Inc. 
 *
 * Sample code interface header for PXA Linux release. All interfaces
 * introduced in this file is not multiple thread safe unless it's 
 * specified explicitly. 
 *
 */



#ifndef	__XSCALE_SOURCE_DEBUG_HEADER__
#define	__XSCALE_SOURCE_DEBUG_HEADER__
#include <stdio.h>

#ifdef	DEBUG_BUILD
	#include <assert.h>
	#define	DPRINTF(fmt,args...)	fprintf(stderr,fmt,##args)
	#define	ASSERT(a)		assert(a)
	#define DBGMSG(fmt,args...)	DPRINTF("PXA: "fmt,##args)
	#define	ENTER()	DPRINTF("PXA: Enter %s\n",__FUNCTION__);	
	#define	LEAVE()	DPRINTF("PXA: Leave %s\n",__FUNCTION__);	
#else
	#define DPRINTF(fmt,args...)	do{}while(0)
	#define DBGMSG(fmt,args...)	do{}while(0)
	#define ASSERT(a)		do{}while(0)
	#define	ENTER()			do{}while(0)
	#define	LEAVE()			do{}while(0)
#endif

#define	ERRMSG(fmt,args...)	fprintf(stderr,"ERR: "fmt,##args)

#endif

