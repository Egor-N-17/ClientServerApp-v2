#include "Headers.h"
#include "Socket.h"

//----------------------------------------------------

class CNetworkUser
{
public://Function
	CNetworkUser() {};
	~CNetworkUser() {};

	virtual wstring Start() = 0;
	virtual void Stop() = 0;
	virtual wstring SetPort(wstring& wstrPort) = 0;
	virtual void GetMessage(CMessage& msg) = 0;
protected://Variables
	BOOL m_bStop;

	std::shared_ptr<CLogger> m_pLogger;
};

//----------------------------------------------------

class CClient : public CNetworkUser
{
public://Function
	CClient();
	~CClient();

	wstring Start();
	void Stop();

	void SetIP(wstring& wstrIP);

	wstring SetPort(wstring& wstrPort);

protected://Function
	wstring StartSendThread();

	void SendThread();

	void GetMessage(CMessage& msg);

	wstring ReadXML();
protected://Variables
	std::shared_ptr<CSocket> m_pClientSocket;
	thread m_thrSend;
	vector<wstring> m_ArrayOfCMD;
	int m_iNumberOfCMD;

	condition_variable m_cvStop;
	mutex m_mutStop;
};

//----------------------------------------------------

class CServer : public CNetworkUser
{
public://Function
	CServer();
	~CServer();

	wstring Start();
	void Stop();

	wstring SetPort(wstring& wstrPort);

protected://Functions
	wstring StartChooseThread();
	void ChooseThread();

	void GetMessage(CMessage& msg);

	void AddClient(socket_ptr& newSock);
	void CloseConnection(socket_ptr& newSock);
	void SendToClient(CMessage& msg);
	void RemoveClient(CMessage& msg);

	void WorkWithQueue();

	wstring ReadXML();
	void LogMessage(CMessage& msg);
protected://Variables
	std::shared_ptr<CSocket> m_pServerSocket;
	vector<std::shared_ptr<CSocket>> m_pClientSockets;
	thread m_thrChoose;

	deque<CMessage> m_dquMainRecv;
	vector<pair<wstring, wstring>> ArrayOfCMDandRES;

	condition_variable m_cvQueueEvent;
	mutex m_mutQueueEvent;

	mutex m_mutUserThread;
};

//----------------------------------------------------

class CNetworkUserFactory
{
public:
	~CNetworkUserFactory() {};

	static std::shared_ptr<CNetworkUserFactory> Instance()
	{
		static std::shared_ptr<CNetworkUserFactory> _instanse(new CNetworkUserFactory());
		return _instanse;
	}

	std::shared_ptr<CNetworkUser> Create(wstring& wstrType)
	{
		if (wstrType == CLIENT_USER)
		{
			return std::make_shared<CClient>();
		}
		if (wstrType == SERVER_USER)
		{
			return std::make_shared<CServer>();
		}

		return nullptr;
	}

protected:
	CNetworkUserFactory() {};
};