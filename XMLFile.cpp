#include "XMLFile.h"

const wchar_t CXMLFile::m_xmlNodeCmd[] = L"cmd";
const wchar_t CXMLFile::m_xmlNodeRes[] = L"res";

//--------------------------------------

CXMLFile::CXMLFile()
{
	m_ifsFile.exceptions(ifstream::failbit);
}

//--------------------------------------

CXMLFile::~CXMLFile()
{
}

//--------------------------------------

wstring CXMLFile::Open(wstring wstrFileName)
{
	if (wstrFileName.empty()) return L"Empty file name string!\n";

	wstring wstrPath = L"";
	wstring wstrError = L"";

	try
	{
		wstrError = GetFilePath(wstrFileName, wstrPath);
		if (wstrError != L"")
		{
			throw FALSE;
		}

		m_ifsFile.clear();
		m_ifsFile.open(ToString(wstrPath), ifstream::in | ios_base::binary);

		if (m_ifsFile.fail())
		{
			wstrError = FormatWText(L"Cannot open file, error: {}\n", GetErrorCode());
			throw FALSE;
		}

		filesystem::path p(wstrPath);
		uintmax_t iFileSize = filesystem::file_size(p);

		if (iFileSize == -1)
		{
			wstrError = FormatWText(L"Cannot get size of file, error: {}\n", GetErrorCode());
			throw FALSE;
		}

		wstrError = Read((size_t)iFileSize);
		if (wstrError != L"")
		{
			throw FALSE;
		}
	}
	catch (BOOL)
	{

	}
	catch (const ifstream::failure& e)
	{
		wstrError = ToWstringError(e.what());
	}

	return wstrError;
}

//--------------------------------------

wstring CXMLFile::GetVectorByKey(wstring wstrKey, vector<wstring>* wVector)
{
	if (wstrKey.empty() || !wVector || m_wptXMLFile.empty()) { assert(false); return L"Invalid pointer\n"; }

	if(m_vReadKey.size()!=0) m_vReadKey.clear();

	wstring wstrError = ReadKey(wstrKey);

	*wVector = m_vReadKey;
	return wstrError;
}

//--------------------------------------

wstring CXMLFile::Read(size_t iFileSize)
{
	assert(m_ifsFile);

	if (iFileSize < 10) return L"Invalid file size!\n";
	bool bIsUnicode = false;
	int iError = 0;

	char* pchBuf = new char[iFileSize + sizeof(wchar_t)];
	memset(pchBuf, 0, iFileSize + sizeof(wchar_t));

	char* pszBuf = { 0 };
	wstringstream wss;
	wstring wstrError = L"";

	bool bEncodFound = false;
	bool bSkipUnicodeByte = false;

	try
	{
		m_ifsFile.read(pchBuf, iFileSize);

		const wchar_t* pFind = wcsstr((const wchar_t*)pchBuf, (const wchar_t*)ENCODING_STRING);
		if (pFind != NULL)
		{
			pFind += sizeof(ENCODING_STRING) - 1;
			if (memcmp(UTF16_WSTRING, pFind, wcslen(UTF16_WSTRING) * 2) == 0)
				bIsUnicode = true;
		}

		if (!bEncodFound || bIsUnicode)
		{
			if ((pchBuf[0] == (char)0xFF && pchBuf[1] == (char)0xFE) ||
				(pchBuf[0] == (char)0xFE && pchBuf[1] == (char)0xFF))
			{
				bIsUnicode = true;
				bSkipUnicodeByte = true;
			}
		}

		if (bIsUnicode)
		{
			int iSkipByte = bSkipUnicodeByte ? WIN(sizeof(wchar_t))NIX(2) : 0;


			WIN(pszBuf = new char[iFileSize * sizeof(wchar_t)];)
			WIN(memset(pszBuf, 0, iFileSize * sizeof(wchar_t));)
			WIN(memcpy(pszBuf, (pchBuf + iSkipByte), iFileSize - iSkipByte);)


			NIX(pszBuf = new char[iFileSize * 2];)
			NIX(memset(pszBuf, 0, iFileSize * 2);)
			NIX(ConvertUTF16LEtoUTF32LE(pchBuf + iSkipByte, iFileSize, pszBuf);) //WORK
			//NIX(memcpy(pszBuf, boost::locale::conv::between(pchBuf + iSkipByte, pchBuf + iFileSize, "utf32le", "utf16le").c_str(), iFileSize * 2);) //WORK
		}
		else
		{
			pszBuf = new char[(iFileSize + 1) * sizeof(wchar_t)];

			memset(pszBuf, 0, (iFileSize + 1) * sizeof(wchar_t));

			ConvertUTF8toWCHAR(pchBuf, (iFileSize + 1) * sizeof(wchar_t), pszBuf);
		}

		wss << (wchar_t*)pszBuf;

		property_tree::read_xml(wss, m_wptXMLFile);

	}
	catch (BOOL)
	{

	}
	catch (const property_tree::ptree_error &e)
	{
		wstrError = ERR_INVALID_XML + ToWstringError(e.what());
	}
	catch (const ifstream::failure& e)
	{
		wstrError = L"Read file error: " + ToWstringError(e.what()) + L"Character extracted " + to_wstring(m_ifsFile.gcount()) + L" from " + to_wstring(iFileSize) + L"\n";
	}


	if (pchBuf) delete[] pchBuf;
	if (pszBuf)
	{
		delete[] pszBuf;
	}

	m_ifsFile.close();

	return wstrError;
}

//--------------------------------------

wstring CXMLFile::ReadKey(wstring wstrKey)
{
	if (wstrKey.empty() || m_wptXMLFile.empty()) { assert(false); return L"Empty key!\n"; }

	wstring wstrError;
	try
	{
		for (auto& node : m_wptXMLFile.get_child(L"Root"))
		{
			if (node.first == wstrKey)
			{
				m_vReadKey.push_back(node.second.get_value<std::wstring>());
			}
		}
	}
	catch (const property_tree::ptree_error &e)
	{
		wstrError = ERR_INVALID_XML + ToWstringError(e.what());
	}

	return wstrError;
}

//--------------------------------------

wstring CXMLFile::GetFilePath(wstring wstrFileName, wstring& wstrPath)
{
	wchar_t wchExeName[MAX_PATH * 2] = { 0 };
	char chExeName[MAX_PATH] = { 0 };
	wstring wstrError = L"";

	try
	{
		filesystem::path p = filesystem::current_path();
		wstrPath = p.wstring();

		wstrPath.append(NIX(L"/")WIN(L"\\") + wstrFileName);
		
		if (filesystem::exists(filesystem::path(ToString(wstrPath))) == false)
		{
			wstrError = L"xml file doesn't exist!\n";
			throw FALSE;
		}
	}
	catch (filesystem::filesystem_error e)
	{
		wstrError = ToWstringError(e.what());
	}
	catch (BOOL)
	{	}

	return wstrError;
}