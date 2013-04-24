/*	cvsnt build number generator
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
// genbuild.cpp : Defines the entry point for the application.
//

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>
#endif
#include <time.h>
#include <stdio.h>

// Increase this if two releases get sent out in the same day
#define BUILD_FROB 0

#ifdef _WIN32
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
#else
int main(int argc, char **argv)
{
	const char *lpCmdLine = argv[1];
#endif

#ifndef _DEBUG
	time_t t = time(NULL);
	size_t days = t/(60*60*24);
	days -= (365*30);
	days += BUILD_FROB;

	FILE *f=fopen(lpCmdLine,"w");
	fprintf(f,"#define CVSNT_PRODUCT_BUILD %d\n",days);
	fclose(f);
#endif
	
	return 0;
}

