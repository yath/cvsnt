#ifndef LISTENSERVER__H
#define LISTENSERVER__H

class CListenServer
{
public:
	CListenServer();
	virtual ~CListenServer();

	bool Listen(LPCSTR Port);
	bool DoServer(SOCKET s);
};

#endif
