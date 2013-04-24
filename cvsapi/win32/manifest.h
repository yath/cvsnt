/*
	CVSNT Generic API - Activate XP controls
    Copyright (C) 2004 Tony Hoyle and March-Hare Software Ltd

	Probably public domain.  From google groups.
*/
/* _EXPORT */
#ifndef MANIFEST__H
#define MANIFEST__H

#ifdef __AFXWIN_H__

class CActivateManifest
{
public:
	CActivateManifest() : m_ulActivationCookie(0), m_hActCtx(0)
	{
		BOOL bRet = Init();

		if (bRet && m_hActCtx && (INVALID_HANDLE_VALUE != m_hActCtx))
			ActivateActCtx(m_hActCtx, &m_ulActivationCookie);
	}

	virtual ~CActivateManifest()
	{
		if (m_hActCtx && (m_hActCtx!= INVALID_HANDLE_VALUE))
		{
			DeactivateActCtx(0, m_ulActivationCookie);
			ReleaseActCtx(m_hActCtx);
		}
	}

private:
	ULONG_PTR m_ulActivationCookie;
	HANDLE m_hActCtx;

	BOOL Init()
	{
		BOOL bRet = FALSE;
		BOOL bTemp = FALSE;

		OSVERSIONINFO info = { 0 };
		info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

		bTemp = GetVersionEx(&info);
		if (bTemp)
		{
			//do special XP theme activation code only on XP or higher...
			if ( (info.dwMajorVersion >= 5) ||
				 (info.dwMajorVersion == 5 && (info.dwMinorVersion >= 1)) &&
				 (info.dwPlatformId == VER_PLATFORM_WIN32_NT))
			{
				ACTCTX actctx = {0};
				TCHAR szModule[MAX_PATH] = {0};

				HINSTANCE hinst = AfxGetInstanceHandle();
				::GetModuleFileName(hinst, szModule, MAX_PATH);

				actctx.cbSize = sizeof(ACTCTX);
				actctx.dwFlags = ACTCTX_FLAG_HMODULE_VALID |
				ACTCTX_FLAG_RESOURCE_NAME_VALID;

				actctx.lpSource = szModule;
				actctx.lpResourceName = MAKEINTRESOURCE(2);
				actctx.hModule = hinst;

				m_hActCtx = ::CreateActCtx(&actctx);
				if (INVALID_HANDLE_VALUE != m_hActCtx)
					bRet = TRUE;
			}
		}

		return bRet;
	}
}; 

#endif

#endif