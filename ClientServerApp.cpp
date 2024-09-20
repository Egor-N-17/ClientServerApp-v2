#include "Headers.h"
#include "User.h"


using namespace std;

//----------------------------------------------------
//			TEST SECTION
//----------------------------------------------------

TEST(void TestUserFactory(void* TestingClass)
{
	wstring wstrType = L"";
	std::shared_ptr<CNetworkUser> userFactory = ((CNetworkUserFactory*)TestingClass)->Create(wstrType);
	AssertEqual(userFactory, nullptr);
})

//----------------------------------------------------

TEST(void TestFactory()
{
	CTestClass tClass(CNetworkUserFactory::Instance().get());
	tClass.RunTest(TestUserFactory, "TestUserFactory");
})

//----------------------------------------------------

TEST(void TestPort(void* TestingClass)
{
	wstring wstrError;
	wstring wstrPort = L"";

	{
		wstrError = ((CClient*)TestingClass)->SetPort(wstrPort);
		AssertEqual(wstrError, ERR_INVALID_PORT);

		wstrPort = L"0";
		wstrError = ((CClient*)TestingClass)->SetPort(wstrPort);
		AssertEqual(wstrError, ERR_INVALID_PORT);

		wstrPort = L"1024";
		wstrError = ((CClient*)TestingClass)->SetPort(wstrPort);
		AssertEqual(wstrError, ERR_INVALID_PORT);

		wstrPort = L"2000";
		wstrError = ((CClient*)TestingClass)->SetPort(wstrPort);
		AssertNotEqual(wstrError, ERR_INVALID_PORT);
	}
})

//----------------------------------------------------

void TestClientIP(void* TestingClass)
{
	wstring wstrError;

	wstring wstrPort = L"2000";
	wstring wstrIP = L"";

	{
		wstrError = ((CNetworkUser*)TestingClass)->SetPort(wstrPort);
		static_cast<CClient*>(((CNetworkUser*)TestingClass))->SetIP(wstrIP);
		wstrError = ((CNetworkUser*)TestingClass)->Start();
		AssertNotEqual(wstrError, L"");
	}

	wstrIP = L"a.b.c.d";

	{
		wstrError = ((CNetworkUser*)TestingClass)->SetPort(wstrPort);
		static_cast<CClient*>(((CNetworkUser*)TestingClass))->SetIP(wstrIP);
		wstrError = ((CNetworkUser*)TestingClass)->Start();
		AssertNotEqual(wstrError, L"");
	}
}

//----------------------------------------------------

TEST(void TestUser()
{
	wstring wstrType;
	{
		wstrType = CLIENT_USER;
		std::shared_ptr<CNetworkUser> pUser = CNetworkUserFactory::Instance()->Create(wstrType);
		CTestClass tClass(pUser.get());
		tClass.RunTest(TestPort, "TestClientPort");
		tClass.RunTest(TestClientIP, "TestClientIP");
	}

	{
		wstrType = SERVER_USER;
		std::shared_ptr<CNetworkUser> pUser = CNetworkUserFactory::Instance()->Create(wstrType);
		CTestClass tClass(pUser.get());
		tClass.RunTest(TestPort, "TestServerPort");
	}
})

//----------------------------------------------------

TEST(void TestXMLFile(void* TestingClass)
{
	wstring wstrError;
	{
		wstrError = ((CXMLFile*)TestingClass)->Open(L"");
		AssertNotEqual(wstrError, L"");
	}
}
)

//----------------------------------------------------

TEST(void TestXML()
{
	CXMLFile xmlFile;
	CTestClass tClass(&xmlFile);
	tClass.RunTest(TestXMLFile, "TestXMLFile");
}
)

//----------------------------------------------------

void GetIP(wstring& wstrIP)
{
	::wprintf(ENTER_IP);
	wchar_t wchIPAddress[15];
	memset(wchIPAddress, 0, 15 * sizeof(wchar_t));
	WIN(::scanf_s("%ls", wchIPAddress, (int)(sizeof(wchIPAddress) / sizeof(wchIPAddress[0])));)
	NIX(scanf("%ls", wchIPAddress);)
	wstrIP = wchIPAddress;
}

//----------------------------------------------------

void GetPort(wstring& wstrPort)
{
	wchar_t wchPort[10];
	memset(wchPort, 0, 10 * sizeof(wchar_t));
	::wprintf(ENTER_PORT);
	WIN(::scanf_s("%ls", wchPort, (int)(sizeof(wchPort) / sizeof(wchPort[0])));)
	NIX(scanf("%ls", wchPort);)
	wstrPort = wchPort;
}

//----------------------------------------------------

void GetUserType(wstring& wstrType)
{
	wchar_t wchType[5];
	memset(wchType, 0, 5 * sizeof(wchar_t));
	::wprintf(L"Choose user type\n1.	Client\n2.	Server\n");
	WIN(::scanf_s("%ls", wchType, (int)(sizeof(wchType) / sizeof(wchType[0])));)
	NIX(scanf("%ls", wchType);)
	wstrType = wchType;
}

//----------------------------------------------------

int main()
{
	::wprintf(L"ClientServerApp\n");

	wstring wstrError;

	std::shared_ptr<CLogger> pLogger = CLogger::Instance();

	wstrError = pLogger->Start();
	if (wstrError != L"")
	{
		wprintf(wstrError.c_str());
		wprintf(ENTER_TO_CLOSE_APP);
		getchar();
		getchar();
	}

	TEST(TestFactory();)
	TEST(TestUser();)
	TEST(TestXML();)

	wstring wstrType;
	wstring wstrIP;
	wstring wstrText;
	GetUserType(wstrType);

	std::shared_ptr<CNetworkUserFactory> userFactory = CNetworkUserFactory::Instance();

	std::shared_ptr<CNetworkUser> pUser;

	while (true)
	{
		if (wstrType == L"1")
		{
			wstrType = CLIENT_USER;
			pUser = userFactory->Create(wstrType);
			GetIP(wstrIP);
			static_cast<CClient*>(pUser.get())->SetIP(wstrIP);
			wstrText = L"Choosen user type: Client\n";
			wstrError = pLogger->LogText(wstrText);
			break;
		}
		else if (wstrType == L"2")
		{
			wstrType = SERVER_USER;
			pUser = userFactory->Create(wstrType);
			wstrText = L"Choosen user type: Server\n";
			wstrError = pLogger->LogText(wstrText);
			break;
		}
		else if (wstrType == L"e")
		{
			wprintf(L"Program stopped by user\n");
			break;
		}
		else
		{
			wprintf(L"Invalid input!Choose 1 for Client or 2 for Server\n");
		}
		GetUserType(wstrType);
	}

	if (pUser == nullptr)
	{
		wstrError = L"Error! Try to reload the programm\n";
	}

	if (wstrError != L"")
	{
		wprintf(wstrError.c_str());
		wprintf(ENTER_TO_CLOSE_APP);
		getchar();
		getchar();
		return 0;
	}

	wstring wstrPort;
	while (true)
	{
		GetPort(wstrPort);
		wstrError = pUser->SetPort(wstrPort);
		if (wstrError == L"")
		{
			break;
		}
		if (wstrError == ERR_INVALID_PORT)
		{
			wprintf(ERR_INVALID_PORT);
			wprintf(L"Port number must be between 1024 and 65535\n");
		}
	}

	wstrError = pUser->Start();
	if (wstrError != L"")
	{
		pUser->Stop();
		wprintf(wstrError.c_str());
		pLogger->LogText(wstrError);
		wprintf(ENTER_TO_CLOSE_APP);
		getchar();
		getchar();
		return 0;
	}


	while (true)
	{
		if (getchar() == 'e')
		{
			pUser->Stop();
			break;
		}
	};


	wprintf(ENTER_TO_CLOSE_APP);

	getchar();
	getchar();


	return 0;
}