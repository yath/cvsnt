#ifndef CVSFLT__H
#define CVSFLT__H

#ifdef __cplusplus
extern "C" {
#endif

BOOL CvsOpenFilter();
BOOL CvsCloseFilter();
BOOL CvsFilterIsOpen();
BOOL CvsAddPosixDirectoryA(LPCSTR dir, BOOL temp);
BOOL CvsAddPosixDirectoryW(LPCWSTR dir, BOOL temp);
BOOL CvsRemovePosixDirectoryA(LPCSTR dir, BOOL temp);
BOOL CvsRemovePosixDirectoryW(LPCWSTR dir, BOOL temp);
BOOL CvsEnumPosixDirectoryA(USHORT Index, LPSTR dir, DWORD cbLen);
BOOL CvsEnumPosixDirectoryW(USHORT Index, LPWSTR dir, DWORD cbLen);

#ifdef _UNICODE
#define CvsAddPosixDirectory CvsAddPosixDirectoryW
#define CvsRemovePosixDirectory CvsRemovePosixDirectoryW
#define CvsEnumPosixDirectory CvsEnumPosixDirectoryW
#else
#define CvsAddPosixDirectory CvsAddPosixDirectoryA
#define CvsRemovePosixDirectory CvsRemovePosixDirectoryA
#define CvsEnumPosixDirectory CvsEnumPosixDirectoryA
#endif

#ifdef __cplusplus
}
#endif

#endif