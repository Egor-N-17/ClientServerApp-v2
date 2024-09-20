#pragma once
#include "string"
#include <iostream>
#include "Headers.h"
#include <source_location>

using namespace std;

//-------------------------------------
//						TEST SECTION
//-------------------------------------
//	To activate unit test uncomment 
//	define of UNIT_TEST

#define UNIT_TEST
#ifdef UNIT_TEST
	#define TEST(exp) exp
#else
	#define TEST(exp)
#endif // UNITEST

//-------------------------------------
//						PLATFORM DEPENDEND CODE
//-------------------------------------

#ifdef _WIN32
	#define WIN(exp) exp
	#define NIX(exp)
#else
	#define WIN(exp)
	#define NIX(exp) exp
#endif

//-------------------------------------
//						FOR ASIO/BOOST
//-------------------------------------

typedef boost::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr;
typedef std::shared_ptr<boost::asio::io_context> context_ptr;
typedef std::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor_ptr;

//-------------------------------------
//						COMMON CONSTANTS
//-------------------------------------

#define ENTER_TO_CLOSE_APP     L"Press Enter to close App\n"
#define ENCODING_STRING	       "encoding=\""
#define ENCODING_WSTRING       L"encoding=\""
#define UTF16_WSTRING          L"UTF-16\""
#define SERVER_XML_FILE        L"ServerFile.xml"
#define CLIENT_XML_FILE        L"ClientFile.xml"
#define INTRO_STRING           L"Make sure, that %ls is in one folder with executable file\n"
#define ENTER_PORT             L"Enter port\n"
#define ENTER_IP               L"Enter IP address\n"
#define XML_OPEN               L"XML file successfully opened!\n"
#define UNKNOWN_CMD            L"Unknown command!"

#define CLIENT_USER            L"Client User"
#define SERVER_USER            L"Server User"

#define CLIENT_SOCKET          L"Client Socket"
#define SERVER_SOCKET          L"Server Socket"

#define NEW_CONNECTION         L"New connection"
#define CLOSE_CONNECTION       L"Connection close"
#define SEND_MESSAGE           L"Send"
#define RECIEVE_MESSAGE        L"Recieve"
#define CONNECTION_TIMEOUT     L"Connection timeout"

#define MAX_SYSTEM_PORT        65535
#define MIN_SYSTEM_PORT        1024
#define SOCK_TIMEOUT_SEC       10
#define MAX_USERS_NUMBER       10
#define INTERVAL_OF_SEND_MS    1000ms
#define INTERVAL_OF_SEND_SEC   1
#define SINCE_1900_YEAR	       1900

#define MSG_FROM_SERVER	       L"Time: {}:{}:{} {}: {} server: {}\n"
#define MSG_FROM_CLIENT	       L"Time: {}:{}:{} {}: {} client: {}\n"
#define LOG_FILE_NAME	       L"LogFile_{}{}{}_{}{}{}.txt"

#define MSG_START              L'\t'
#define MSG_DELIMITER          L'\n'

#define LOCALHOST_IP           L"127.0.0.1"

//-------------------------------------
//						ERROR STRINGS
//-------------------------------------

#define ERR_INVALID_PORT L"Invalid port\n"
#define ERR_INVALID_XML  L"Invalid xml file: "

//-------------------------------------

NIX(
typedef struct sockaddr_in	SOCKADDR_IN;
typedef unsigned int		SOCKET;
#define SD_BOTH				0

typedef bool				BOOL;
typedef unsigned long		DWORD;
#define FALSE				false
#define TRUE				true
#define LPCWSTR				wchar_t*

#define MAX_PATH			1024
#define SOCKET_ERROR		-1
)
//-------------------------------------
//						WORK WITH STRINGS
//-------------------------------------

template<typename... Args>
inline std::wstring FormatWText(std::wstring wstrFormat, Args&&... args)
{
	return std::vformat(wstrFormat, std::make_wformat_args(args...));
}

//-------------------------------------

inline wstring ToWstring(string strText)
{
	wchar_t wchResult[MAX_PATH * sizeof(wchar_t)];
	memset(wchResult, 0, MAX_PATH * sizeof(wchar_t));
	size_t sChConverted;
	WIN(errno_t)NIX(error_t) result =	WIN(mbstowcs_s(&sChConverted, wchResult, MAX_PATH*sizeof(wchar_t), &strText[0], MAX_PATH))
										NIX(mbstowcs(wchResult, &strText[0], MAX_PATH));
	if (result == -1)
	{
		wprintf(L"ToWstring error!\n");
	}
	return wchResult;
}

//-------------------------------------

inline wstring ToWstringError(string strError)
{
	if (strError[strError.length() - 1] != '\n')
	{
		strError.push_back('\n');
	}
	return ToWstring(strError);
}

//-------------------------------------

inline string ToString(wstring wstrText)
{
	char chResult[MAX_PATH];
	memset(chResult, 0, MAX_PATH);
	size_t sChConverted;
	WIN(errno_t)NIX(error_t) result =	WIN(wcstombs_s(&sChConverted, chResult, MAX_PATH, &wstrText[0], MAX_PATH))
										NIX(wcstombs(chResult, &wstrText[0], MAX_PATH));
	if (result == -1)
	{
		wprintf(L"ToString error!\n");
	}
	return chResult;
}

//-------------------------------------

NIX(
inline wstring WstrBoostError(const boost::system::error_code& err)
{
	return ToWstring(err.message() + " code: " + err.category().name() + ":" + to_string(err.value()));
})

//-------------------------------------
//						GET ERROR CODE
//-------------------------------------

inline int GetErrorCode()
{
	return WIN(::GetLastError()) NIX(errno);
}
//-------------------------------------

inline int GetSocketErrorCode()
{
	return WIN(::WSAGetLastError()) NIX(errno);
}

//-------------------------------------
//						WORK WITH MEMORY
//-------------------------------------

template<typename T> class CFreeMem
{
public:
	CFreeMem(int iNumber)
	{
		p = new T[iNumber];
	}
	~CFreeMem()
	{
		delete[] p;
	}
public:
	T *p;
};

//-------------------------------------
//						FUNCTIONS
//-------------------------------------

NIX(
inline int _wtoi(const wchar_t *wstr)
{
	return (int)wcstol(wstr, 0, 10);
}
)

//-------------------------------------

NIX(inline void ConvertEnconding(string strTo, string strFrom, char* pchIn, size_t iSizeIn, char* pchOut, size_t iSizeOut)
{
	memcpy(pchOut, boost::locale::conv::between(pchIn, pchIn + iSizeIn, strTo, strFrom).c_str(), iSizeOut);
})

//-------------------------------------


NIX(inline void ConvertUTF16LEtoUTF32LE(char* pchIn, size_t iSizeIn, char* pchOut)
{ 
	memcpy(pchOut, boost::locale::conv::between(pchIn, pchIn + iSizeIn, "utf32le", "utf16le").c_str(), iSizeIn*2);
})

//-------------------------------------

NIX(inline void ConvertUTF32LEtoUTF16LE(char* pchIn, size_t iSizeIn, char* pchOut)
{
	memcpy(pchOut, boost::locale::conv::between(pchIn, pchIn + iSizeIn, "utf16le", "utf32le").c_str(), iSizeIn/2);
})

//-------------------------------------

inline void ConvertUTF8toWCHAR(char* pchIn, size_t iSizeIn, char* pchOut)
{
	std::locale loc("");
	std::locale conv_loc = boost::locale::util::create_info(loc, loc.name());
	memcpy(pchOut, boost::locale::conv::to_utf<wchar_t>(pchIn, conv_loc).c_str(), iSizeIn);
}

//-------------------------------------

inline tm GetTime()
{
	chrono::time_point<chrono::system_clock> now = chrono::system_clock::now();
	time_t time = chrono::system_clock::to_time_t(now);
	tm tm;

	WIN(localtime_s(&tm, &time);)
	NIX(localtime_r(&time, &tm);)

	return tm;
}

//-------------------------------------
//						LOG MACROS
//-------------------------------------

#define LOG_STRING	   L"{} ({}:{}) : {}"

#define _TR(exp)            { wstring wstrError = exp; if(wstrError.empty() == false) throw CreateLogString(wstrError, std::source_location::current()); }

inline wstring CreateLogString(wstring wstrText, std::source_location location)
{
	wstring wstrMessage = FormatWText(LOG_STRING, ToWstring(location.file_name()).c_str(), location.line(), location.column(), wstrText.c_str());
	
	return wstrMessage;
}
