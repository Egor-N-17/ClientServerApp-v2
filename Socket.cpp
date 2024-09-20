#include "Socket.h"
#include "User.h"

using namespace std;

//----------------------------------------------------
//	CSocket
//----------------------------------------------------

wstring CSocket::SetPort(wstring& wstrPort)
{
	int iPort = _wtoi(wstrPort.c_str());

	if (MAX_SYSTEM_PORT <= iPort || MIN_SYSTEM_PORT >= iPort)
	{
		return ERR_INVALID_PORT;
	}

	m_shPort = (short)iPort;

	return L"";
}

//----------------------------------------------------

wstring CSocket::GetSocket()
{
	return m_wstrSocket;
}

//------------------------------------------

wstring CSocket::GetLocalIP(string& strIP)
{
	wstring wstrError;

#ifdef _WIN32

	PMIB_IPADDRTABLE pIPAddrTable;

	try
	{
		DWORD dwSize = 0;
		DWORD dwRetVal = 0;

		pIPAddrTable = (MIB_IPADDRTABLE*)HeapAlloc(GetProcessHeap(), 0, (sizeof(MIB_IPADDRTABLE)));

		if (pIPAddrTable)
		{
			if (GetIpAddrTable(pIPAddrTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER)
			{
				HeapFree(GetProcessHeap(), 0, (pIPAddrTable));
				pIPAddrTable = (MIB_IPADDRTABLE*)HeapAlloc(GetProcessHeap(), 0, dwSize);
			}
			if (pIPAddrTable == NULL) {
				wstrError = FormatWText(L"Memory allocation failed for GetIpAddrTable\n");
				throw FALSE;
			}
		}

		if ((dwRetVal = GetIpAddrTable(pIPAddrTable, &dwSize, 0)) != NO_ERROR)
		{
			wstrError = FormatWText(L"GetIpAddrTable failed with error {}\n", dwRetVal);
			throw FALSE;
		}

		IN_ADDR IPAddr;
		IPAddr.S_un.S_addr = (u_long)pIPAddrTable->table[0].dwAddr;
		char IP1[16];
		strIP = inet_ntop(AF_INET, &IPAddr, IP1, 16);
	}
	catch (BOOL)
	{

	}

	if (pIPAddrTable)
	{
		HeapFree(GetProcessHeap(), 0, (pIPAddrTable));
	}

#else
	ifaddrs* ifaddr, * ifa;
	int family, s, n;
	char host[NI_MAXHOST];

	try
	{
		if (getifaddrs(&ifaddr) == -1)
		{
			wstrError = FormatWText(L"getifaddrs failed with error: {}\n", GetSocketErrorCode());
			throw FALSE;
		}

		for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++)
		{
			if (ifa->ifa_addr == NULL)
			{
				continue;
			}
			family = ifa->ifa_addr->sa_family;
			if (family == AF_INET && string(ifa->ifa_name).find("en") != string::npos)
			{
				s = getnameinfo(ifa->ifa_addr,
					(family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
					host,
					NI_MAXHOST,
					NULL,
					0,
					NI_NUMERICHOST);

				if (s != 0)
				{
					wstrError = FormatWText(L"getnameinfo failed with error: {}\n", s);
					throw FALSE;
				}
				strIP = host;
			}
		}
	}
	catch (BOOL)
	{

	}

	if (ifaddr != NULL) freeifaddrs(ifaddr);
#endif

	return wstrError;
}

//----------------------------------------------------
//	CClientSocket
//----------------------------------------------------

CClientSocket::CClientSocket(CNetworkUser* pUser)
{
	m_bStop = FALSE;
	m_bSocketConnected = FALSE;
	m_pParentObject = pUser;
}

//----------------------------------------------------

CClientSocket::~CClientSocket()
{
	Stop();
}

//----------------------------------------------------

wstring CClientSocket::Start()
{
	try
	{
		if (m_pSocket == nullptr)
		{
			_TR(InitSocket());

			_TR(StartContextThread());
		}
		else
		{
			m_bSocketConnected = TRUE;

			InitMainTimers();

			StartRecv();
			WaitOutput();

			m_wstrSocket = to_wstring((int)m_pSocket->native_handle());
		}
	}
	catch (wstring wstrError)
	{
		return wstrError;
	}

	return L"";
}

//----------------------------------------------------

void CClientSocket::SetIP(wstring& wstrIP)
{
	m_wstrIP = wstrIP;
}

//----------------------------------------------------

void CClientSocket::SetSocket(socket_ptr sock)
{
	m_pSocket = sock;
}

//----------------------------------------------------

void CClientSocket::SetIOContext(context_ptr context)
{
	m_pIOContext = context;
}

//----------------------------------------------------

void CClientSocket::SendMessage(wstring wstrText)
{
	if (m_bStop) return;
	m_mutMessage.lock();
	m_wstrMessageQueue.push_back(wstrText);
	m_mutMessage.unlock();
	if (m_bSocketConnected == FALSE) return;
	m_timerSend->expires_from_now(boost::posix_time::seconds(INTERVAL_OF_SEND_SEC));
}

//----------------------------------------------------

wstring CClientSocket::InitSocket()
{
	wstring wstrError = L"";
	string strIP;
	bool bError = FALSE;

	try
	{
		m_pIOContext = std::make_shared<boost::asio::io_context>();

		m_pSocket = socket_ptr(new boost::asio::ip::tcp::socket(*m_pIOContext));

		if (m_wstrIP == LOCALHOST_IP)
		{
			_TR(GetLocalIP(strIP));
		}
		else
		{
			strIP = ToString(m_wstrIP);
		}

		boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string(strIP), m_shPort);

		m_timerConnect = make_shared<boost::asio::deadline_timer>(*m_pIOContext);

		m_timerConnect->expires_from_now(boost::posix_time::seconds(SOCK_TIMEOUT_SEC));

		m_pSocket->async_connect(ep, boost::bind(&CClientSocket::HandleConnect, this, boost::asio::placeholders::error, ep));

		m_timerConnect->async_wait(boost::bind(&CClientSocket::CheckConnect, this));
	}
	catch (std::exception& e)
	{
		wstrError = ToWstringError(e.what());
		bError = TRUE;
	}
	catch (wstring wstrError)
	{
		bError = TRUE;
	}

	if (bError)
	{
		m_pSocket.reset();
		m_pIOContext.reset();
	}

	return wstrError;
}

//----------------------------------------------------

void CClientSocket::CheckRecv()
{
	if (m_bStop)
		return;

	if (m_timerRecv->expires_at() <= boost::asio::deadline_timer::traits_type::now())
	{
		m_bStop = TRUE;
		CMessage msg = CMessage(CLOSE_CONNECTION, L"Recvieve timeout", m_wstrSocket);
		m_pParentObject->GetMessage(msg);
	}
	else
	{
		m_timerRecv->async_wait(boost::bind(&CClientSocket::CheckRecv, this));
	}
}

//----------------------------------------------------

void CClientSocket::CheckConnect()
{
	if (m_bStop)
		return;

	if (m_timerConnect->expires_at() <= boost::asio::deadline_timer::traits_type::now() && m_bSocketConnected == FALSE)
	{
		m_bStop = TRUE;
		CMessage msg = CMessage(CLOSE_CONNECTION, L"Connection timeout", m_wstrSocket);
		m_pParentObject->GetMessage(msg);
	}
	else
	{
		if (m_bSocketConnected == FALSE)
		{
			m_timerConnect->async_wait(boost::bind(&CClientSocket::CheckConnect, this));
		}
	}
}

//----------------------------------------------------

void CClientSocket::WaitOutput()
{
	if (m_bStop)
		return;

	m_mutMessage.lock();
	BOOL bEmpty = m_wstrMessageQueue.empty();
	m_mutMessage.unlock();

	if (bEmpty)
	{
		m_timerSend->expires_from_now(boost::posix_time::seconds(INTERVAL_OF_SEND_SEC));
		m_timerSend->async_wait(
			boost::bind(&CClientSocket::WaitOutput, this));
	}
	else
	{
		StartSend();
	}
}

//----------------------------------------------------

void CClientSocket::HandleConnect(const boost::system::error_code& ec, boost::asio::ip::tcp::endpoint ep)
{
	if (m_bStop)
		return;

	if (!m_pSocket->is_open())
	{
		m_bStop = TRUE;
		CMessage msg = CMessage(CLOSE_CONNECTION, CONNECTION_TIMEOUT, m_wstrSocket);
		m_pParentObject->GetMessage(msg);
	}

	else if (ec)
	{
		m_bStop = TRUE;
		CMessage msg = CMessage(CLOSE_CONNECTION, WIN(ToWstring(ec.what())) NIX(WstrBoostError(ec)), m_wstrSocket);
		m_pParentObject->GetMessage(msg);
	}
	else
	{
		wprintf(L"Connection opened\n");
		m_wstrSocket = to_wstring((int)m_pSocket->native_handle());

		m_bSocketConnected = TRUE;

		InitMainTimers();

		StartRecv();
		WaitOutput();
	}

}

//----------------------------------------------------

void CClientSocket::InitMainTimers()
{
	m_timerRecv = make_shared<boost::asio::deadline_timer>(*m_pIOContext);

	m_timerSend = make_shared<boost::asio::deadline_timer>(*m_pIOContext);
}

//----------------------------------------------------

wstring CClientSocket::StartContextThread()
{
	wstring wstrError;
	try
	{
		m_thrContext = thread(&CClientSocket::ContextThread, this);
	}
	catch (std::exception e)
	{
		wstrError = ToWstringError(e.what());
	}
	return wstrError;
}

//----------------------------------------------------

void CClientSocket::ContextThread()
{
	m_pIOContext->run();
}

//----------------------------------------------------

void CClientSocket::StartRecv()
{
	m_timerRecv->expires_from_now(boost::posix_time::seconds(SOCK_TIMEOUT_SEC));

	boost::asio::async_read_until(*m_pSocket.get(), m_OutBuf, MSG_DELIMITER,
		boost::bind(&CClientSocket::HandleRecv, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));

	m_timerRecv->async_wait(boost::bind(&CClientSocket::CheckRecv, this));
}

//----------------------------------------------------

void CClientSocket::HandleRecv(const boost::system::error_code& err, size_t bytesTransfered)
{
	if (m_bStop)
		return;

	if (!err)
	{
		wstring wstrText;

		ExtractMessage(wstrText);

		CMessage msg = CMessage(RECIEVE_MESSAGE, wstrText, m_wstrSocket);
		m_pParentObject->GetMessage(msg);

		StartRecv();
	}
	else
	{
		m_bStop = TRUE;
		CMessage msg = CMessage(CLOSE_CONNECTION, WIN(ToWstring(err.what())) NIX(WstrBoostError(err)), m_wstrSocket);
		m_pParentObject->GetMessage(msg);
	}
}

//----------------------------------------------------

void CClientSocket::ExtractMessage(wstring& wstrText)
{
	const int iBufLen = 512;

	CFreeMem<char> pchRecvBuf(iBufLen * sizeof(wchar_t));
	memset(pchRecvBuf.p, 0, iBufLen * sizeof(wchar_t));

	auto buffer = m_OutBuf.data();
	const char* first = asio::buffer_cast<const char*>(buffer);
	size_t bufsiz = asio::buffer_size(buffer);
	const char* last = first + bufsiz;

	auto start = std::find(first, last, MSG_START) + WIN(sizeof(wchar_t))NIX(2);
	auto end = std::find(first, last, MSG_DELIMITER);

	NIX(CFreeMem<char> recvbuf(iBufLen);)
	NIX(memcpy(recvbuf.p, start, distance(start, end));)
	NIX(ConvertUTF16LEtoUTF32LE(recvbuf.p, distance(start, end), pchRecvBuf.p);)

	WIN(memcpy(pchRecvBuf.p, start, distance(start, end));)

	auto to_consume = std::min(std::size_t(std::distance(first, end) + 1), bufsiz);
	m_OutBuf.consume(to_consume);

	wstrText = (wchar_t*)pchRecvBuf.p;

	size_t pos = wstrText.find(MSG_DELIMITER);
	if (pos != wstring::npos)
	{
		wstrText.erase(wstrText.begin() + pos);
	}
}

//----------------------------------------------------

void CClientSocket::StartSend()
{
	m_timerSend->expires_from_now(boost::posix_time::seconds(INTERVAL_OF_SEND_SEC));

	wstring wstrSend;
	m_mutMessage.lock();
	wstrSend = MSG_START + m_wstrMessageQueue.front() + MSG_DELIMITER;
	m_mutMessage.unlock();

	PutMessage(wstrSend);

	boost::asio::async_write(*m_pSocket.get(),
		m_InBuf,
		boost::bind(&CClientSocket::HandleSend, this, boost::asio::placeholders::error));
}

//----------------------------------------------------

void CClientSocket::PutMessage(wstring& wstrText)
{
	int iLenInByte = ((int)wstrText.length() + 1) * sizeof(wchar_t);
	CFreeMem<char> pchSendBuf(NIX(iLenInByte / 2)WIN(iLenInByte));

	NIX(CFreeMem<char> pchSendText(iLenInByte);)
	NIX(::memcpy(pchSendText.p, wstrText.c_str(), iLenInByte);)
	NIX(ConvertUTF32LEtoUTF16LE(pchSendText.p, iLenInByte, pchSendBuf.p);)

	WIN(::memcpy(pchSendBuf.p, wstrText.c_str(), iLenInByte);)

	auto buffer = m_InBuf.prepare(WIN(iLenInByte)NIX(iLenInByte / 2));
	std::copy(pchSendBuf.p, pchSendBuf.p + WIN(iLenInByte)NIX(iLenInByte / 2), asio::buffer_cast<char*>(buffer));
	m_InBuf.commit(WIN(iLenInByte)NIX(iLenInByte / 2));
}

//----------------------------------------------------

void CClientSocket::HandleSend(const boost::system::error_code& err)
{
	if (m_bStop)
		return;

	CMessage msg;

	if (!err)
	{
		m_mutMessage.lock();
		wstring wstrText = m_wstrMessageQueue.front();
		m_wstrMessageQueue.pop_front();
		m_mutMessage.unlock();

		msg = CMessage(SEND_MESSAGE, wstrText.c_str(), m_wstrSocket);
		m_pParentObject->GetMessage(msg);


		WaitOutput();
	}
	else
	{
		m_bStop = TRUE;
		msg = CMessage(CLOSE_CONNECTION, WIN(ToWstring(err.what())) NIX(WstrBoostError(err)), m_wstrSocket);
		m_pParentObject->GetMessage(msg);
	}
}

//----------------------------------------------------

void CClientSocket::Stop()
{
	m_bStop = TRUE;
	if (m_timerSend)
	{
		m_timerSend->expires_from_now(boost::posix_time::seconds(INTERVAL_OF_SEND_SEC));
	}

	if (m_bSocketConnected && m_pSocket && m_pSocket->is_open())
	{
		m_pSocket->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
		m_pSocket->close();
	}

	if (m_timerRecv)
	{
		m_timerRecv->cancel();
	}

	if (m_timerSend)
	{
		m_timerSend->cancel();
	}

	if (m_timerConnect)
	{
		m_timerConnect->cancel();
	}

	if (m_thrContext.joinable())
	{
		if (m_pIOContext && m_pIOContext->stopped() == false)
		{
			m_pIOContext->stop();
		}

		m_thrContext.join();
	}

	wprintf(L"All threads are stopped\n");
}

//----------------------------------------------------
//	CServerSocket
//----------------------------------------------------

CServerSocket::CServerSocket(CNetworkUser* pUser)
{
	m_bStop = FALSE;
	m_pParentObject = pUser;
}

//------------------------------------------

CServerSocket::~CServerSocket()
{
	Stop();
}

//------------------------------------------

wstring CServerSocket::Start()
{
	wstring wstrError;

	try
	{
		_TR(InitSocket());

		_TR(StartContextThread();)
	}
	catch(wstring wstrError)
	{
		return wstrError;
	}

	return L"";
}

//----------------------------------------------------

void CServerSocket::SetIP(wstring& wstrIP)
{
	m_wstrIP = wstrIP;
}

//----------------------------------------------------

context_ptr CServerSocket::GetIOContext()
{
	return m_pIOContext;
}

//------------------------------------------

wstring CServerSocket::InitSocket()
{
	wstring wstrError = L"";
	bool bError = FALSE;

	try
	{
		m_pIOContext = std::make_shared<boost::asio::io_context>();

		string strIP;
		_TR(GetLocalIP(strIP));
		
		m_Acceptor = acceptor_ptr(new boost::asio::ip::tcp::acceptor(*m_pIOContext, boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(strIP), m_shPort)));

		m_wstrSocket = to_wstring((int)m_Acceptor->native_handle());

		socket_ptr newSock(new boost::asio::ip::tcp::socket(*m_pIOContext));
		StartAccept(newSock);
	}
	catch (std::exception& e)
	{
		wstrError = ToWstringError(e.what());
		bError = TRUE;
	}
	catch (wstring wstrError)
	{
		bError = TRUE;
	}

	if (bError)
	{
		m_pIOContext.reset();
		m_Acceptor.reset();
	}

	return wstrError;
}

//------------------------------------------

void CServerSocket::StartAccept(socket_ptr sock)
{
	m_Acceptor->async_accept(*sock, boost::bind(&CServerSocket::HandleAccept, this, sock, boost::asio::placeholders::error));
}

//------------------------------------------

void CServerSocket::HandleAccept(socket_ptr sock, const boost::system::error_code& err)
{
	CMessage msg;

	if (err)
	{
		msg = CMessage(CLOSE_CONNECTION, WIN(ToWstring(err.what())) NIX(WstrBoostError(err)), m_wstrSocket);
		m_pParentObject->GetMessage(msg);
		return;
	}

	msg = CMessage(NEW_CONNECTION, sock, m_wstrSocket);
	m_pParentObject->GetMessage(msg);
	
	socket_ptr newSock(new boost::asio::ip::tcp::socket(*m_pIOContext));
	StartAccept(newSock);
}

//------------------------------------------

void CServerSocket::ContextThread()
{
	m_pIOContext->run();
}

//------------------------------------------

wstring CServerSocket::StartContextThread()
{
	wstring wstrError;
	try
	{
		m_thrContext = thread(&CServerSocket::ContextThread, this);
	}
	catch (std::exception e)
	{
		wstrError = ToWstringError(e.what());
	}
	return wstrError;
}

//------------------------------------------

void CServerSocket::Stop()
{
	m_bStop = TRUE;

	if (m_Acceptor && m_Acceptor->is_open())
	{
		m_Acceptor->close();
	}
	
	if (m_pIOContext && m_pIOContext->stopped() == false)
	{
		m_pIOContext->stop();
	}

	if (m_thrContext.joinable())
	{
		m_thrContext.join();
	}

	::wprintf(L"Server stopped\n");
}
