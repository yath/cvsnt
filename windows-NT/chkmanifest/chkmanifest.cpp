/*
	CVSNT Automatic manifest generator
    Copyright (C) 2006 Tony Hoyle and March-Hare Software Ltd

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

// chkmanifest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../../version.h"

static bool debug;

struct manifestStruct
{
	manifestStruct() { found = false; }
	_bstr_t version,name,type,processor,language;
	bool found;
};

typedef std::vector<manifestStruct> manifestArray;

int checkSub(const char *szDll, std::map<std::string, int> & toCheck);

int load_manifest(const char *xml, manifestArray& array)
{
	MSXML2::IXMLDOMDocument2Ptr doc;
	
	doc.CreateInstance("Msxml2.DOMDocument.6.0");

	if(doc==NULL)
	{
		printf("Unable to create MSXML instance\n");
		exit(1);
	}

	if(!doc->loadXML(xml))
		return -1;

	doc->setProperty("SelectionLanguage","XPath");
	doc->setProperty("SelectionNamespaces", "xmlns:ns='urn:schemas-microsoft-com:asm.v1'");

	_bstr_t version,name,type,processor,language;
	MSXML2::IXMLDOMNodePtr node,val;

	node = doc->selectSingleNode("/ns:assembly/ns:assemblyIdentity");
	if(node==NULL)
		return -1;
	val = node->attributes->getNamedItem("version");
	if(val) version = val->text;
	else return -1;

	val = node->attributes->getNamedItem("name");
	if(val) name = val->text;
	else return -1;

	val = node->attributes->getNamedItem("processorArchitecture");
	if(val) processor = val->text;
	else processor="*";

	val = node->attributes->getNamedItem("language"); // No examples of this so this may be wrong
	if(val) language = val->text;
	else language="*";

	val = node->attributes->getNamedItem("type");
	if(val) type = val->text;
	else type="Win32";

	if(debug)
	{
		printf("name: %s\n",(const char *)name);
		printf("version: %s\n",(const char *)version);
		printf("architecture: %s\n",(const char *)processor);
		printf("language: %s\n",(const char *)language);
		printf("type: %s\n",(const char *)type);
	}

	MSXML2::IXMLDOMNodeListPtr nodeList;

	nodeList = doc->selectNodes("/ns:assembly/ns:dependency/ns:dependentAssembly/ns:assemblyIdentity");

	array.clear();
	for(short n=0; n<nodeList->length; n++)
	{
		node = nodeList->item[n];

		val = node->attributes->getNamedItem("version");
		if(val) version = val->text;
		else return -1;

		val = node->attributes->getNamedItem("name");
		if(val) name = val->text;
		else return -1;

		val = node->attributes->getNamedItem("processorArchitecture");
		if(val) processor = val->text;
		else processor="*";

		val = node->attributes->getNamedItem("language"); // No examples of this so this may be wrong
		if(val) language = val->text;
		else language="*";

		val = node->attributes->getNamedItem("type");
		if(val) type = val->text;
		else type="Win32";

		if(debug)
		{
			printf("\n");
			printf("depname: %s\n",(const char *)name);
			printf("depversion: %s\n",(const char *)version);
			printf("deparchitecture: %s\n",(const char *)processor);
			printf("deplanguage: %s\n",(const char *)language);
			printf("deptype: %s\n",(const char *)type);
		}

		manifestStruct str;
		str.name = name;
		str.version = version;
		str.processor = processor;
		str.language = language;
		str.type = type;
		array.push_back(str);
	}
	return 0;
}

int parse_manifest(const char *xml, manifestArray& array, const char *szPath)
{
	MSXML2::IXMLDOMDocument2Ptr doc;
	
	doc.CreateInstance("Msxml2.DOMDocument.6.0");
	if(doc==NULL)
	{
		printf("Unable to create MSXML instance\n");
		exit(1);
	}

	if(!doc->loadXML(xml))
		return -1;

	doc->setProperty("SelectionLanguage","XPath");
	doc->setProperty("SelectionNamespaces", "xmlns:ns='urn:schemas-microsoft-com:asm.v1'");

	_bstr_t version,name,type,processor,language;
	MSXML2::IXMLDOMNodePtr node,val;

	node = doc->selectSingleNode("/ns:assembly/ns:assemblyIdentity");
	if(node==NULL)
		return -1;
	val = node->attributes->getNamedItem("version");
	if(val) version = val->text;
	else return -1;

	val = node->attributes->getNamedItem("name");
	if(val) name = val->text;
	else return -1;

	val = node->attributes->getNamedItem("processorArchitecture");
	if(val) processor = val->text;
	else processor="*";

	val = node->attributes->getNamedItem("language"); // No examples of this so this may be wrong
	if(val) language = val->text;
	else language="*";

	val = node->attributes->getNamedItem("type");
	if(val) type = val->text;
	else type="Win32";

	if(debug)
	{
		printf("name: %s\n",(const char *)name);
		printf("version: %s\n",(const char *)version);
		printf("architecture: %s\n",(const char *)processor);
		printf("language: %s\n",(const char *)language);
		printf("type: %s\n",(const char *)type);
	}

	bool error = false;
	for(manifestArray::iterator i = array.begin(); i!=array.end(); ++i)
	{
		if(strcmp(i->name,name))
			continue;
		if(strcmp(i->version,"*") && strcmp(i->version,version))
		{
			printf("  ** Incorrect version: Expected %s found %s\n",(const char *)i->version,(const char *)version);
			error = true;
		}
		if(strcmp(i->processor,"*") && strcmp(i->processor,processor))
		{
			printf("  ** Incorrect processor: Expected %s found %s\n",(const char *)i->processor,(const char *)processor);
			error = true;
		}
		if(strcmp(i->language,"*") && strcmp(i->language,language))
		{
			printf("  ** Incorrect language: Expected %s found %s\n",(const char *)i->language,(const char *)language);
			error = true;
		}
		if(strcmp(i->type,"*") && strcmp(i->type,type))
		{
			printf("  ** Incorrect type: Expected %s found %s\n",(const char *)i->type,(const char *)type);
		}
		i->found = true;
	}

	return error?1:0;
}
 
static WORD g_wManifestLanguage=MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);

BOOL CALLBACK EnumResLangProc(      
        HANDLE hModule,
        LPCTSTR lpszType,
        LPCTSTR lpszName,
        WORD wIDLanguage,
        LONG_PTR lParam)
{
	g_wManifestLanguage=wIDLanguage;
	return FALSE;
}

int main(int argc, char* argv[])
{
	std::map<std::string, int> toCheck, beenChecked;
	CoInitialize(NULL);

	debug=false;

	while(argc>1 && argv[1][0]=='-')
	{
		switch(argv[1][1])
		{
		case 'd':
			debug = true;
			break;
		default:
			printf("cvsnt manifest checker "CVSNT_PRODUCTVERSION_STRING"\n");
			printf("usage: chkmanifest <file>\n");
			return -1;
		}
		argv++;
		argc--;
	}

	if(argc<2)
	{
		printf("cvsnt manifest checker "CVSNT_PRODUCTVERSION_STRING"\n");
		printf("usage: chkmanifest <file>\n");
		return -1;
	}


	bool done;
	int err = 0;

	toCheck.clear();
	err += checkSub(argv[1],toCheck);
	do
	{
		done = false;
		for(std::map<std::string, int>::const_iterator i = toCheck.begin(); i!=toCheck.end(); ++i)
		{
			if(beenChecked[i->first]++)
				continue;
			err += checkSub(i->first.c_str(), toCheck);
			done = true;
			break;
		}
	} while(done);

	if(err)
	{
		printf("** there were errors!  Program may not run correctly\n");
	}
}

int checkSub(const char *szDll, std::map<std::string, int> & toCheck)
{
	LOADED_IMAGE *loaded_image;
	char szPath[4096];
	manifestArray array;

	HMODULE hRes = LoadLibraryEx(szDll,NULL,DONT_RESOLVE_DLL_REFERENCES);
	if(!hRes)
	{
		printf("Couldn't load image %s!\n",szDll);
		return 0;
	}

	GetModuleFileName(hRes, szPath, sizeof(szPath));

	printf("Module %s\n\n",szPath);

	HRSRC hRsrc = FindResource(hRes,MAKEINTRESOURCE(1),RT_MANIFEST);
	if(!hRsrc)
		hRsrc = FindResource(hRes,MAKEINTRESOURCE(2),RT_MANIFEST);
	if(!hRsrc)
		hRsrc = FindResource(hRes,MAKEINTRESOURCE(3),RT_MANIFEST);
	if(hRsrc)
	{
		EnumResourceLanguages(hRes,RT_MANIFEST,MAKEINTRESOURCE(1),(ENUMRESLANGPROC)EnumResLangProc,NULL);
		LPVOID pRsrc = LoadResource(hRes, hRsrc);
		DWORD dwLen = SizeofResource(hRes, hRsrc);
		DWORD dwOldProtect;
		VirtualProtect(pRsrc,dwLen+1,PAGE_READWRITE,&dwOldProtect);
		((LPBYTE)pRsrc)[dwLen]='\0'; // This is cheating but we don't care since this is the only data we need
		if(load_manifest((const char *)pRsrc, array))
		{
			if(debug)
				printf("%s\n",pRsrc);
			printf("  ** Invalid manifest\n");
			FreeResource(hRsrc);
			return 1;
		}
		FreeResource(hRsrc);
	}
	else
	{
		if(debug)
			printf("  ** No manifest\n");
		return 0;
	}

	loaded_image=ImageLoad((PSTR)szDll,NULL);
	if(!loaded_image)
	{
		printf("  **  Couldn't load image!\n");
		return 0;
	}

	DWORD	dwSize;
	PBYTE      pDllName;
	int err = 0;

	PIMAGE_IMPORT_DESCRIPTOR pimage_import_descriptor= (PIMAGE_IMPORT_DESCRIPTOR)
		ImageDirectoryEntryToData(loaded_image->MappedAddress,FALSE,IMAGE_DIRECTORY_ENTRY_IMPORT,&dwSize);

	printf("Dependencies:\n");
	while(pimage_import_descriptor->Name!=0)
	{
		pDllName = (PBYTE)ImageRvaToVa(loaded_image->FileHeader,loaded_image->MappedAddress,pimage_import_descriptor->Name,NULL);

		HMODULE hRes = LoadLibraryEx((LPCSTR)pDllName,NULL,DONT_RESOLVE_DLL_REFERENCES);
		if(hRes)
		{
			HRSRC hRsrc = FindResource(hRes,MAKEINTRESOURCE(1),RT_MANIFEST);
			GetModuleFileName(hRes, szPath, sizeof(szPath));
			if(hRsrc)
			{
				LPVOID pRsrc = LoadResource(hRes, hRsrc);
				DWORD dwLen = SizeofResource(hRes, hRsrc);
				DWORD dwOldProtect;
				VirtualProtect(pRsrc,dwLen+1,PAGE_READWRITE,&dwOldProtect);
				((LPBYTE)pRsrc)[dwLen]='\0'; // This is cheating but we don't care since this is the only data we need
				printf("  %s\n",szPath);
				err+=parse_manifest((const char *)pRsrc, array, szPath);
				FreeResource(hRsrc);
			}
			else
				printf("  %s\n",szPath);
			FreeLibrary(hRes);
			toCheck[szPath]++;
		}
		else
		{
			printf("  %s **MISSING**\n",pDllName);
			err++;
		}

		pimage_import_descriptor++;
	}

	ImgDelayDescr *pimage_delay_descriptor= (ImgDelayDescr*)
		ImageDirectoryEntryToData(loaded_image->MappedAddress,FALSE,IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT,&dwSize);

	printf("\nDelayload dependencies:\n");
	while(pimage_delay_descriptor && pimage_delay_descriptor->rvaDLLName!=0)
	{
		pDllName = (PBYTE)ImageRvaToVa(loaded_image->FileHeader,loaded_image->MappedAddress,pimage_delay_descriptor->rvaDLLName,NULL);

		HMODULE hRes = LoadLibraryEx((LPCSTR)pDllName,NULL,DONT_RESOLVE_DLL_REFERENCES);
		if(hRes)
		{
			GetModuleFileName(hRes, szPath, sizeof(szPath));
			HRSRC hRsrc = FindResource(hRes,MAKEINTRESOURCE(1),RT_MANIFEST);
			if(hRsrc)
			{
				LPVOID pRsrc = LoadResource(hRes, hRsrc);
				DWORD dwLen = SizeofResource(hRes, hRsrc);
				DWORD dwOldProtect;
				VirtualProtect(pRsrc,dwLen+1,PAGE_READWRITE,&dwOldProtect);
				((LPBYTE)pRsrc)[dwLen]='\0'; // This is cheating but we don't care since this is the only data we need
				printf("  %s\n",szPath);
				err+=parse_manifest((const char *)pRsrc, array, szPath);
				FreeResource(hRsrc);
			}
			else
				printf("  %s\n",szPath);
			FreeLibrary(hRes);
			toCheck[szPath]++;
		}
		else
			printf("  %s ** MISSING **\n",pDllName); // This is not an error since it's delayload

	    pimage_delay_descriptor++;
	}
	printf("\n");

	for(manifestArray::const_iterator i = array.begin(); i!=array.end(); ++i)
	{
		if(i->found)
			continue;
		printf("  **  Module dependency missing - %s version %s\n",(const char *)i->name,(const char *)i->version);
		err++;
	}

	printf("\n");

	return err;
}

