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

// mkmanifest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../../version.h"

int parse_manifest(MSXML2::IXMLDOMDocument2Ptr outDoc, const char *xml)
{
	MSXML2::IXMLDOMDocument2Ptr doc;
	
	doc.CreateInstance("Msxml2.DOMDocument.6.0");

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
#ifdef _WIN64
	else processor="amd64";
#else
	else processor="x86";
#endif

	val = node->attributes->getNamedItem("language"); // No examples of this so this may be wrong
	if(val) language = val->text;
	else language="*";

	val = node->attributes->getNamedItem("type");
	if(val) type = val->text;
	else type="Win32";

	node = outDoc->selectSingleNode("/ns:assembly");
	val = outDoc->createNode(MSXML2::NODE_ELEMENT,"dependency","urn:schemas-microsoft-com:asm.v1");
	node->appendChild(val);
	node = val;

	MSXML2::IXMLDOMNodePtr dep = outDoc->createNode(MSXML2::NODE_ELEMENT,"dependentAssembly","urn:schemas-microsoft-com:asm.v1");
	MSXML2::IXMLDOMNodePtr iden = outDoc->createNode(MSXML2::NODE_ELEMENT,"assemblyIdentity","urn:schemas-microsoft-com:asm.v1");

	node->appendChild(dep);
	dep->appendChild(iden);

	val = outDoc->createAttribute("version");
	val->text=version;
	iden->attributes->setNamedItem(val);

	val = outDoc->createAttribute("type");
	val->text=type;
	iden->attributes->setNamedItem(val);

	val = outDoc->createAttribute("name");
	val->text=name;
	iden->attributes->setNamedItem(val);

	val = outDoc->createAttribute("processorArchitecture");
	val->text=processor;
	iden->attributes->setNamedItem(val);

	val = outDoc->createAttribute("language");
	val->text=language;
	iden->attributes->setNamedItem(val);

	return 0;
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
	LOADED_IMAGE *loaded_image;
	char szPath[4096];
	std::string version,path;
	bool debug;

	CoInitialize(NULL);

	MSXML2::IXMLDOMDocument2Ptr doc;
	
	doc.CreateInstance("Msxml2.DOMDocument.6.0");

	version=CVSNT_PRODUCTVERSION_SHORT;
	path="..;d:\\cvsbin\\sysfiles;d:\\cvsbin";
	debug=false;

	while(argc>1 && argv[1][0]=='-')
	{
		switch(argv[1][1])
		{
		case 'v':
			version = argv[2];
			argv++;
			argc--;
			break;
		case 'P':
			path = argv[2];
			argv++;
			argc--;
			break;
		case 'd':
			debug = true;
			break;
		default:
			printf("cvsnt manifest patcher "CVSNT_PRODUCTVERSION_STRING"\n");
			printf("usage: mkmanifest [-v version] [-P searchpath] [-d] <library>\n");
			return -1;
		}
		argv++;
		argc--;
	}

	if(argc<2)
	{
		printf("cvsnt manifest generator "CVSNT_PRODUCTVERSION_STRING"\n");
		printf("usage: mkmanifest [-v version] [-P searchpath] [-d] <library>\n");
		return -1;
	}

	HMODULE hRes = LoadLibraryEx(argv[1],NULL,DONT_RESOLVE_DLL_REFERENCES);
	if(!hRes)
	{
		printf("Couldn't load image!\n");
		return 0;
	}

	GetModuleFileName(hRes, szPath, sizeof(szPath));

	if(debug)
		printf("Generating manifest for version %s of %s\n",version.c_str(),szPath);

	*strrchr(szPath,'\\')='\0';
	SetCurrentDirectory(szPath);

	strcpy(szPath,path.c_str());
	strcat(szPath,";");
	GetEnvironmentVariable("Path",szPath+strlen(szPath),(DWORD)(sizeof(path)-strlen(szPath)));
	SetEnvironmentVariable("Path",szPath);

	HRSRC hRsrc = FindResource(hRes,MAKEINTRESOURCE(1),RT_MANIFEST);
	if(hRsrc)
	{
		EnumResourceLanguages(hRes,RT_MANIFEST,MAKEINTRESOURCE(1),(ENUMRESLANGPROC)EnumResLangProc,NULL);
		LPVOID pRsrc = LoadResource(hRes, hRsrc);
		DWORD dwLen = SizeofResource(hRes, hRsrc);
		DWORD dwOldProtect;
		VirtualProtect(pRsrc,dwLen+1,PAGE_READWRITE,&dwOldProtect);
		((LPBYTE)pRsrc)[dwLen]='\0'; // This is cheating but we don't care since this is the only data we need
		if(doc->loadXML((const char *)pRsrc))
		{
			doc->setProperty("SelectionLanguage","XPath");
			doc->setProperty("SelectionNamespaces", "xmlns='urn:schemas-microsoft-com:asm.v1' xmlns:ns='urn:schemas-microsoft-com:asm.v1'");
		}
		else
		{
			const char *_xml="<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
						"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">\n"
						"<assemblyIdentity version=\"1.0.0.0\" processorArchitecture=\"*\" name=\"\" type=\"win32\"/>\n"
						"</assembly>\n";
			if(doc->loadXML(_xml))
			{
				doc->setProperty("SelectionLanguage","XPath");
				doc->setProperty("SelectionNamespaces", "xmlns='urn:schemas-microsoft-com:asm.v1' xmlns:ns='urn:schemas-microsoft-com:asm.v1'");
			}
		}
		FreeResource(hRsrc);
	}
	else
	{
		const char *_xml="<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
					"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">\n"
					"<assemblyIdentity version=\"1.0.0.0\" processorArchitecture=\"*\" name=\"\" type=\"win32\"/>\n"
					"</assembly>\n";
		if(doc->loadXML(_xml))
		{
			doc->setProperty("SelectionLanguage","XPath");
			doc->setProperty("SelectionNamespaces", "xmlns='urn:schemas-microsoft-com:asm.v1' xmlns:ns='urn:schemas-microsoft-com:asm.v1'");
		}
	}
/*	hRsrc = FindResource(hRes,MAKEINTRESOURCE(1),RT_VERSION);
	if(hRsrc)
	{
		LPVOID pRsrc = LoadResource(hRes, hRsrc);
		// something here.. updating versioninfo is even harder than updating the manifests!
		FreeResource(hRsrc);
	}*/

	loaded_image=ImageLoad(argv[1],NULL);
	if(!loaded_image)
	{
		printf("Couldn't load image!");
		return 0;
	}

	DWORD	dwSize;
	PBYTE      pDllName;

	PIMAGE_IMPORT_DESCRIPTOR pimage_import_descriptor= (PIMAGE_IMPORT_DESCRIPTOR)
		ImageDirectoryEntryToData(loaded_image->MappedAddress,FALSE,IMAGE_DIRECTORY_ENTRY_IMPORT,&dwSize);

	if(debug) printf("Dependencies:\n");
	while(pimage_import_descriptor->Name!=0)
	{
		pDllName = (PBYTE)ImageRvaToVa(loaded_image->FileHeader,loaded_image->MappedAddress,pimage_import_descriptor->Name,NULL);

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
				if(debug) printf("    %s (manifest)\n",szPath);
				parse_manifest(doc, (const char *)pRsrc);
				FreeResource(hRsrc);
			}
			else
				if(debug) printf("    %s\n",szPath);
			FreeLibrary(hRes);
		}

		pimage_import_descriptor++;
	}

	ImgDelayDescr *pimage_delay_descriptor= (ImgDelayDescr*)
		ImageDirectoryEntryToData(loaded_image->MappedAddress,FALSE,IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT,&dwSize);

	if(debug) printf("Delayload dependencies:\n");
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
				if(debug) printf("    %s (manifest)\n",szPath);
				parse_manifest(doc, (const char *)pRsrc);
				FreeResource(hRsrc);
			}
			else
				if(debug) printf("    %s\n",szPath);
			FreeLibrary(hRes);
		}

	    pimage_delay_descriptor++;
	}

	MSXML2::IXMLDOMNodePtr node,val;
	std::string name,arch;
	size_t off;
	const char *pname;

	pname = strrchr(loaded_image->ModuleName,'\\');
	if(!pname) pname = strrchr(loaded_image->ModuleName,'/');
	if(!pname) pname=loaded_image->ModuleName;
	else pname++;
	name = pname;
	off = name.find_last_of('.');
	if(off!=std::string::npos)
		name.resize(off);

#ifdef _WIN64
	arch="amd64";
#else
	arch="x86";
#endif

	node = doc->selectSingleNode("/ns:assembly/ns:assemblyIdentity");
	if(node==NULL)
	{
		printf("processing error: no assemblyIdentity\n");
		return -1;
	}
	val = node->attributes->getNamedItem("version");
	if(val) val->text=version.c_str();
	else
	{
		val = doc->createAttribute("version");
		val->text=version.c_str();
		node->attributes->setNamedItem(val);
	}
	val = node->attributes->getNamedItem("name");
	if(val) val->text=name.c_str();
	else
	{
		val = doc->createAttribute("name");
		val->text=name.c_str();
		node->attributes->setNamedItem(val);
	}
	val = node->attributes->getNamedItem("processorArchitecture");
	if(val) val->text=arch.c_str();
	else
	{
		val = doc->createAttribute("processorArchitecture");
		val->text=arch.c_str();
		node->attributes->setNamedItem(val);
	}

	ImageUnload(loaded_image);

	const char *_xsl = 
			"<xsl:stylesheet version=\"1.0\"\n"
			"	xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\">\n"
			"<xsl:output method=\"xml\"/>\n"
			"<xsl:param name=\"indent-increment\" select=\"'   '\" />\n"
			"\n"
			"<xsl:template match=\"*\">\n"
			"	<xsl:param name=\"indent\" select=\"'&#xA;'\"/>\n"
			"\n"
			"	<xsl:value-of select=\"$indent\"/>\n"
			"	<xsl:copy>\n"
			"		<xsl:copy-of select=\"@*\" />\n"
			"		<xsl:apply-templates>\n"
			"			<xsl:with-param name=\"indent\"\n"
			"				select=\"concat($indent, $indent-increment)\"/>\n"
			"		</xsl:apply-templates>\n"
			"		<xsl:value-of select=\"$indent\"/>\n"
			"   </xsl:copy>\n"
			"</xsl:template>\n"
			"\n"
			"<xsl:template match=\"comment()|processing-instruction()\">\n"
			"	<xsl:copy />\n"
			"</xsl:template>\n"
			" \n"
			"<!-- WARNING: this is dangerous. Handle with care -->\n"
			"<xsl:template match=\"text()[normalize-space(.)='']\"/>\n"
			"\n"
			"</xsl:stylesheet>\n";

	MSXML2::IXMLDOMDocument2Ptr xslDoc;
	xslDoc.CreateInstance("Msxml2.DOMDocument.6.0");
	xslDoc->loadXML(_xsl);
	_bstr_t txt = doc->transformNode(xslDoc);

	if(debug)
	{
		printf("\n");
		printf("%s\n",(const char *)txt);
	}

	GetModuleFileName(hRes,szPath,MAX_PATH);
	FreeLibrary(hRes);

	HANDLE hUpdate = BeginUpdateResource(szPath,FALSE);
	if(!hUpdate)
	{
		printf("BeginUpdateResource failed!!!\n");
		return -1;
	}
	if(!UpdateResource(hUpdate,RT_MANIFEST,MAKEINTRESOURCE(1),g_wManifestLanguage,(LPVOID)(const char *)txt,txt.length()))
	{
		printf("UpdateResource failed!!\n");
		return -1;
	}
	if(!EndUpdateResource(hUpdate, FALSE))
	{
		printf("EndUpdateResource failed!!\n");
		return -1;
	}

	getchar();
	return 0;
}

