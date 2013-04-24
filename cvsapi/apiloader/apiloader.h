/*
	CVSNT Generic API
    Copyright (C) 2004-5 Tony Hoyle and March-Hare Software Ltd

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License version 2.1 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/* Win32 specific */
/* _EXPORT */
#ifndef APILOADER__H
#define APILOADER__H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _DELAY_IMP_VER
extern void *__pfnDliNotifyHook2;
extern void *__pfnDliFailureHook2;
#endif

void __apiloadHookDelayFunctions(void **notify, void **failure);

/* Call this once to alter search path of delayload before any dlls load */
#define InitApiLoader() { __apiloadHookDelayFunctions((void**)&__pfnDliNotifyHook2,(void**)&__pfnDliFailureHook2); }

#ifdef __cplusplus
}
#endif

#endif
