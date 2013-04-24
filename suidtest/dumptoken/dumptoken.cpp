// dumptoken.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

BOOL MyGetTokenInformation(HANDLE hToken, TOKEN_INFORMATION_CLASS Class, LPVOID *pBuffer)
{
	DWORD dwLen=0;
	GetTokenInformation(hToken,Class,NULL,0,&dwLen);
	if(!dwLen) return FALSE;

	*pBuffer=malloc(dwLen);
	return GetTokenInformation(hToken,Class,*pBuffer,dwLen,&dwLen);
}

BOOL SidToString(HANDLE hLsa, PSID Sid, char *string)
{
	LSA_TRANSLATED_NAME *name;
	LSA_REFERENCED_DOMAIN_LIST *list;
	LPTSTR szSid;
	NTSTATUS stat;
	if((stat=LsaLookupSids(hLsa,1,&Sid,&list,&name))!=ERROR_SUCCESS && stat!=STATUS_SOME_NOT_MAPPED && stat!=STATUS_NONE_MAPPED)
	{
		SetLastError(LsaNtStatusToWinError(stat));
		return FALSE;
	}
	ConvertSidToStringSid(Sid,&szSid);
	if(list->Entries && list->Domains->Name.Length)
		sprintf(string,"%-*.*S\\%-*.*S (%s)",list->Domains->Name.Length/2,list->Domains->Name.Length/2,list->Domains->Name.Buffer,name->Name.Length/2,name->Name.Length/2,name->Name.Buffer,szSid);
	else
		sprintf(string,"%-*.*S (%s)",name->Name.Length/2,name->Name.Length/2,name->Name.Buffer,szSid);
	LocalFree(szSid);
	LsaFreeMemory(name);
	LsaFreeMemory(list);
	return TRUE;
}

int main(int argc, char* argv[])
{
	HANDLE hToken,hLsa;
	TOKEN_GROUPS_AND_PRIVILEGES *ptgap;
	TOKEN_DEFAULT_DACL *ptddacl;
	TOKEN_OWNER *ptowner;
	TOKEN_PRIMARY_GROUP *ptpgroup;
	TOKEN_USER *ptuser;
	char name[512];
	char *szDesc;

	LSA_OBJECT_ATTRIBUTES oa = {0};

	OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&hToken);
	LsaOpenPolicy(NULL,&oa,POLICY_LOOKUP_NAMES,&hLsa);

	MyGetTokenInformation(hToken,TokenGroupsAndPrivileges,(LPVOID*)&ptgap);
	MyGetTokenInformation(hToken,TokenDefaultDacl,(LPVOID*)&ptddacl);
	MyGetTokenInformation(hToken,TokenOwner,(LPVOID*)&ptowner);
	MyGetTokenInformation(hToken,TokenPrimaryGroup,(LPVOID*)&ptpgroup);
	MyGetTokenInformation(hToken,TokenUser,(LPVOID*)&ptuser);

	printf("Groups (%d):\n\n",ptgap->SidCount);
	for(size_t n=0; n<ptgap->SidCount; n++)
	{
		if(SidToString(hLsa,ptgap->Sids[n].Sid, name))
			printf("%s\n",name);
	}

	printf("\nPrivileges (%d):\n\n",ptgap->PrivilegeCount);

	for(size_t n=0; n<ptgap->PrivilegeCount; n++)
	{
		DWORD dwName=sizeof(name);
		if(LookupPrivilegeName(NULL,&ptgap->Privileges[n].Luid,name,&dwName))
			printf("%s\n",name);
	}

	printf("\nOwner:\n\n");

	SidToString(hLsa,ptowner->Owner,name);
	printf("%s\n",name);

	printf("\nPrimary Group:\n\n");

	SidToString(hLsa,ptpgroup->PrimaryGroup,name);
	printf("%s\n",name);

	printf("\nUser:\n\n");

	SidToString(hLsa,ptuser->User.Sid,name);
	printf("%s\n",name);

	printf("\nDacl:\n\n");

	ULONG dwLen = sizeof(name);
	ConvertSecurityDescriptorToStringSecurityDescriptor(ptddacl->DefaultDacl,SDDL_REVISION_1,DACL_SECURITY_INFORMATION|SACL_SECURITY_INFORMATION|OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION,&szDesc,&dwLen);
	printf("%s\n",name);
	LocalFree(szDesc);

	free(ptgap);
	free(ptddacl);
	free(ptowner);
	free(ptpgroup);
	free(ptuser);
	LsaClose(hLsa);
	CloseHandle(hToken);

	return 0;
}

