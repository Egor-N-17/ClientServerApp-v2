#include "Headers.h"

using namespace std;

class CNetworkUser;

//----------------------------------------------------

class CMessage
{
public:
	CMessage(wstring wstrOper, wstring wstrInfo, wstring wstrSocket) :
		wstrOperation(wstrOper), wstrSocket(wstrSocket)
	{
		info = wstrInfo;
	};
	CMessage(wstring wstrOper, socket_ptr sock, wstring wstrSocket) :
		wstrOperation(wstrOper), wstrSocket(wstrSocket)
	{
		info = sock;
	};
	CMessage()
	{
		wstrOperation = L"";
		info.reset();
		wstrSocket = L"";
	}
	~CMessage() {};

	bool empty()
	{
		return wstrOperation.empty() && info.has_value() == false && wstrSocket.empty();
	}

	wstring wstrOperation;
	std::any info;
	wstring wstrSocket;
};

//----------------------------------------------------
#pragma once
class CSocket
{
public://Functions
	CSocket() {};
	~CSocket() {};

	virtual wstring Start() = 0;
	virtual void Stop() = 0;

	virtual void SetIP(wstring& wstrIP) = 0;
	wstring SetPort(wstring& wstrPort);

	wstring GetSocket();

protected://Functions
	virtual wstring InitSocket() = 0;
	wstring GetLocalIP(string& strIP);
public://Variables

protected://Variables
	BOOL m_bStop;

	short m_shPort;
	wstring m_wstrIP;
	wstring m_wstrSocket;	// used only for log message

	CNetworkUser* m_pParentObject;

	context_ptr m_pIOContext;

	mutex m_mutMessage;
};

//----------------------------------------------------

class CClientSocket : public CSocket
{
public://Functions
	CClientSocket(CNetworkUser* pUser);
	~CClientSocket();

	wstring Start();
	void Stop();

	void SetIP(wstring& wstrIP);
	void SetSocket(socket_ptr sock);
	void SetIOContext(context_ptr context);
	void SendMessage(wstring wstrText);

protected://Functions
	wstring InitSocket();

private://Functions
	void StartRecv();
	void StartSend();

	void HandleRecv(const boost::system::error_code& err, size_t bytesTransfered);
	void HandleSend(const boost::system::error_code& err);

	void CheckRecv();
	void CheckConnect();
	void WaitOutput();

	void HandleConnect(const boost::system::error_code& ec, boost::asio::ip::tcp::endpoint ep);

	void InitMainTimers();

	wstring StartContextThread();
	void ContextThread();

	void ExtractMessage(wstring& wstrText);
	void PutMessage(wstring& wstrText);
public://Variables

private://Variables
	thread m_thrContext;

	deque<wstring> m_wstrMessageQueue;

	socket_ptr m_pSocket;

	BOOL m_bSocketConnected;

	std::shared_ptr<boost::asio::deadline_timer> m_timerRecv;
	std::shared_ptr<boost::asio::deadline_timer> m_timerSend;
	std::shared_ptr<boost::asio::deadline_timer> m_timerConnect;

	asio::streambuf m_OutBuf;
	asio::streambuf m_InBuf;
};

//----------------------------------------------------

class CServerSocket : public CSocket
{
public://Functions
	CServerSocket(CNetworkUser* pUser);
	~CServerSocket();

	wstring Start();
	void Stop();

	void SetIP(wstring& strIP);
	context_ptr GetIOContext();

private://Functions
	wstring InitSocket();

	void StartAccept(socket_ptr sock);
	void HandleAccept(socket_ptr sock, const boost::system::error_code& err);

	void ContextThread();
	wstring StartContextThread();
public://Variables

private://Variables

	thread m_thrContext;
	acceptor_ptr m_Acceptor;
};

//----------------------------------------------------

class CSocketFactory
{
public:
	~CSocketFactory() {};

	static std::shared_ptr<CSocketFactory> Instance()
	{
		static std::shared_ptr<CSocketFactory> _instanse(new CSocketFactory());
		return _instanse;
	}

	std::shared_ptr<CSocket> Create(wstring wstrType, CNetworkUser* pUser)
	{
		if (wstrType == CLIENT_SOCKET)
		{
			return std::make_shared<CClientSocket>(pUser);
		}
		if (wstrType == SERVER_SOCKET)
		{
			return std::make_shared<CServerSocket>(pUser);
		}

		return nullptr;
	}

protected:
	CSocketFactory() {};
};