/*
	CVSNT Generic API
    Copyright (C) 2004 Tony Hoyle and March-Hare Software Ltd

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
#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>
#include <tchar.h>

#include <config.h>
#include <stdio.h>

#include "../lib/api_system.h"
#include "../cvs_string.h"
#include "../FileAccess.h"

bool CFileAccess::m_bUtf8Mode = false;

CFileAccess::CFileAccess()
{
	m_file = NULL;
}

CFileAccess::~CFileAccess()
{
	close();
}

bool CFileAccess::open(const char *filename, const char *mode)
{
	m_file = _wfopen(Win32Wide(filename),Win32Wide(mode));
	if(!m_file)
		return false;
	return true;
}

bool CFileAccess::open(FILE *file)
{
	if(m_file)
		return false;
	m_file = file;
	return true;
}

bool CFileAccess::isopen()
{
	if(!m_file)
		return false;
	return true;
}

bool CFileAccess::close()
{
	if(m_file)
		fclose(m_file);
	m_file = NULL;
	return true;
}

bool CFileAccess::getline(cvs::string& line)
{
	int c;

	if(!m_file)
		return false;

	line.reserve(256);
	line="";

	while((c=fgetc(m_file))!=EOF)
	{
		if(c=='\n')
			break;
		if(c=='\r')
			continue;
		line.append(1,c);
	}
	if(c==EOF && line.empty())
		return false;
	return true;
}

bool CFileAccess::getline(char *line, size_t length)
{
	int c;
	size_t len = length;

	if(!m_file)
		return false;

	while(len && (c=fgetc(m_file))!=EOF)
	{
		if(c=='\n')
			break;
		*(line++)=(char)c;
		--len;
	}
	if(c==EOF && len==length)
		return false;
	return true;
}

bool CFileAccess::putline(const char *line)
{
	if(!m_file)
		return false;

	if(fwrite(line,1,strlen(line),m_file)<strlen(line))
		return false;
	if(fwrite("\n",1,1,m_file)<1)
		return false;
	return true;
}

size_t CFileAccess::read(void *buf, size_t length)
{
	if(!m_file)
		return 0;

	return fread(buf,1,length,m_file);
}

size_t CFileAccess::write(const void *buf, size_t length)
{
	if(!m_file)
		return 0;

	return fwrite(buf,length,1,m_file);
}

loff_t CFileAccess::length()
{
	if(!m_file)
		return 0;

	fpos_t pos,len;

	if(fgetpos(m_file,&pos)<0)
		return 0;
	fseek(m_file,0,SEEK_END);
	if(fgetpos(m_file,&len)<0)
		return 0;
	if(fsetpos(m_file,&pos)<0)
		return 0;
	return (loff_t)len;
}

loff_t CFileAccess::pos()
{
	if(!m_file)
		return 0;

	fpos_t o;
	if(fgetpos(m_file,&o)<0)
		return 0;
	return (loff_t)o;
}

bool CFileAccess::eof()
{
	if(!m_file)
		return false;

	return feof(m_file)?true:false;
}

bool CFileAccess::seek(loff_t pos, SeekEnum whence)
{
	if(!m_file)
		return false;

	fpos_t p;

	switch(whence)
	{
	case seekBegin:
		p=(fpos_t)pos;
		break;
	case seekCurrent:
		if(fgetpos(m_file,&p)<0)
			return false;
		p+=pos;
		break;
	case seekEnd:
		if(pos>0x7FFFFFFF)
		{
			if(fseek(m_file,0x7FFFFFFF,whence)<0)
				return false;
			pos-=0x7FFFFFFFF;
			while(pos>0x7FFFFFFF && !fseek(m_file,-0x7FFFFFFF,SEEK_CUR))
				pos-=0x7FFFFFFFF;
			if(pos>0x7FFFFFFF)
				return false;
			pos=-pos;
		}
		if(fseek(m_file,(long)pos,SEEK_CUR)<0)
			return false;
		return true;
	default:
		return false;
	}
	if(fsetpos(m_file,&p)<0)
		return false;
	return true;
}

cvs::wstring CFileAccess::wtempdir()
{
	cvs::wstring dir;
	DWORD fa;

	dir.resize(MAX_PATH);
	if(!GetEnvironmentVariableW(_T("TEMP"),(TCHAR*)dir.data(),(DWORD)dir.size()) &&
	     !GetEnvironmentVariableW(_T("TMP"),(TCHAR*)dir.data(),(DWORD)dir.size()))
	{
		// No TEMP or TMP, use default <windir>\TEMP
		GetWindowsDirectoryW((wchar_t*)dir.data(),(UINT)dir.size());
		wcscat((wchar_t*)dir.data(),L"\\TEMP");
	}
	dir.resize(wcslen((wchar_t*)dir.data()));
	if((fa=GetFileAttributesW(dir.c_str()))==0xFFFFFFFF || !(fa&FILE_ATTRIBUTE_DIRECTORY))
	{
		// Last resort, can't find a valid temp.... use C:\...
		dir=L"C:\\";
	}
	return dir;
}

cvs::string CFileAccess::tempdir()
{
	return (const char *)Win32Narrow(wtempdir().c_str());
}

cvs::string CFileAccess::tempfilename(const char *prefix)
{
	cvs::wstring tempfile;
	tempfile.resize(MAX_PATH);
	GetTempFileNameW(wtempdir().c_str(),Win32Wide(prefix),0,(wchar_t*)tempfile.data());
	tempfile.resize(wcslen((wchar_t*)tempfile.data()));
	DeleteFileW(tempfile.c_str());
	return (const char *)Win32Narrow(tempfile.c_str());
}

bool CFileAccess::_remove(cvs::wstring& path)
{
	WIN32_FIND_DATAW wfd;
	HANDLE hFind;

	hFind = FindFirstFileW((path+L"/*.*").c_str(),&wfd);
	if(hFind==INVALID_HANDLE_VALUE)
		return FALSE;
	do
	{
		if(wfd.cFileName[0]!='.' || (wfd.cFileName[1]!='.' && wfd.cFileName[1]!='\0'))
		{
			size_t l = path.length();
			path+=L"/";
			path+=wfd.cFileName;
			if(wfd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
			{
				if(!_remove(path))
				{
					FindClose(hFind);
					return false;
				}
			}
			else
			{
				if(wfd.dwFileAttributes&FILE_ATTRIBUTE_READONLY)
					SetFileAttributesW(path.c_str(),wfd.dwFileAttributes&~FILE_ATTRIBUTE_READONLY);
				if(!DeleteFile(path.c_str()))
				{
					FindClose(hFind);
					return false;
				}
			}
			path.resize(l);
		}
	} while(FindNextFile(hFind,&wfd));
	FindClose(hFind);
	if(!RemoveDirectoryW(path.c_str()))
		return false;
	return true;
}

bool CFileAccess::remove(const char *file, bool recursive /* = false */)
{
	Win32Wide wfile(file);
	DWORD att = GetFileAttributesW(wfile);
	if(att==0xFFFFFFFF)
		return false;
	SetFileAttributesW(wfile,att&~FILE_ATTRIBUTE_READONLY);
	if(att&FILE_ATTRIBUTE_DIRECTORY)
	{
		if(!recursive)
		{
			if(!RemoveDirectoryW(wfile))
				return false;
		}
		else
		{
			cvs::wstring wpath = wfile;
			if(!_remove(wpath))
				return false;
		}
	}
	else
	{
		if(!DeleteFileW(wfile))
			return false;
	}
	return true;
}

bool CFileAccess::rename(const char *from, const char *to)
{
	Win32Wide wfrom(from);
	Win32Wide wto(to);
	DWORD att = GetFileAttributesW(wfrom);
	if(att==0xFFFFFFFF)
		return false;
	SetFileAttributesW(wfrom,att&~FILE_ATTRIBUTE_READONLY);
	att = GetFileAttributesW(wto);
	if(att!=0xFFFFFFFF)
		SetFileAttributesW(wto,att&~FILE_ATTRIBUTE_READONLY);
	if(!MoveFileExW(wfrom,wto,MOVEFILE_COPY_ALLOWED|MOVEFILE_REPLACE_EXISTING))
		return false;
	return true;
}

void CFileAccess::Win32SetUtf8Mode(bool bUtf8Mode)
{
	m_bUtf8Mode = bUtf8Mode;
}

CFileAccess::Win32Wide::Win32Wide(const char *fn)
{
	if(!fn)
	{
		pbuf=NULL;
		return;
	}
	size_t l = strlen(fn)+1;
	if(l>(sizeof(buf)/sizeof(wchar_t)))
		pbuf=new wchar_t[l];
	else
		pbuf=buf;
	MultiByteToWideChar(m_bUtf8Mode?CP_UTF8:CP_ACP,0,fn,(int)l,pbuf,(int)l*sizeof(wchar_t));
}

CFileAccess::Win32Wide::~Win32Wide()
{
	if(pbuf && pbuf!=buf)
		delete[] pbuf;
}

CFileAccess::Win32Narrow::Win32Narrow(const wchar_t *fn)
{
	if(!fn)
	{
		pbuf=NULL;
		return;
	}
	size_t l = wcslen(fn)+1;
	if((l*3)>sizeof(buf))
		pbuf=new char[l*3];
	else
		pbuf=buf;
	WideCharToMultiByte(m_bUtf8Mode?CP_UTF8:CP_ACP,0,fn,(int)l,pbuf,(int)(l*3),NULL,NULL);
}

CFileAccess::Win32Narrow::~Win32Narrow()
{
	if(pbuf && pbuf!=buf)
		delete[] pbuf;
}

bool CFileAccess::exists(const char *file)
{
	if(GetFileAttributesW(Win32Wide(file))==0xFFFFFFFF)
		return false;
	return true;
}

CFileAccess::TypeEnum CFileAccess::type(const char *file)
{
	DWORD dw = GetFileAttributesW(Win32Wide(file));
	if(dw==0xFFFFFFFF)
		return typeNone;
	if(dw&FILE_ATTRIBUTE_DIRECTORY)
		return typeDirectory;
	return typeFile;
}

bool CFileAccess::absolute(const char *file)
{
	return file[0]=='\\' || file[0]=='/' || file[1]==':';
}

int CFileAccess::uplevel(const char *file)
{
	int level = 0;
	for(const char *p=file; *p;)
	{
		size_t l=strcspn(p,"\\/");
		if(l==1 && *p=='.')
			level++; // Compensate for ./
		else if(l==2 && *p=='.' && *(p+1)=='.')
			level+=2; // Compensate for ../
		p+=l;
		if(*p) p++;
		level--;
	}
	return level;
}

cvs::string CFileAccess::mimetype(const char *filename)
{
	HKEY hk;

	const char *p = strrchr(filename,'.');
	if(!p)
		return "";

	if(RegOpenKeyEx(HKEY_CLASSES_ROOT,cvs::wide(p),0,KEY_QUERY_VALUE,&hk))
		return "";

	TCHAR str[256];
	DWORD len = sizeof(str)-sizeof(TCHAR);
	if(RegQueryValueEx(hk,L"Content Type",NULL,NULL,(LPBYTE)str,&len))
	{
		RegCloseKey(hk);
		return "";
	}
	str[len]='\0';
	return (const char *)cvs::narrow(str);
}
