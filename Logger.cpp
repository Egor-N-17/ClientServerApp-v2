#include "Logger.h"

//--------------------------------------

wstring CLogger::Start()
{
	wstring wstrPath = L"";

	try
	{
		_TR(CreateFolder(wstrPath));

		_TR(CreateFile(wstrPath));
	}
	catch (wstring wstrError)
	{
		return wstrError;
	}
	return L"";
}

//--------------------------------------

wstring CLogger::LogText(wstring& wstrText)
{
	if (wstrText.empty()|| !m_wofsLogFile.good())
	{
		return L"Invalid pointer. Can't write string in log file\n";
	}

	wstring wstrError = L"";

	try
	{
		m_wofsLogFile.write(wstrText.c_str(), wstrText.length());
	}
	catch (const wofstream::failure& e)
	{
		wstrError = ToWstringError(e.what());
	}

	return wstrError;
}

//--------------------------------------

wstring CLogger::CreateFolder(wstring& wstrPath)
{
	wstring wstrError = L"";

	try
	{
		filesystem::path p = filesystem::current_path();
		wstrPath = p.wstring();

		wstrPath.append(NIX(L"/")WIN(L"\\") + wstring(L"LogFiles"));

		if (filesystem::exists(filesystem::path(ToString(wstrPath))) == true)
		{
			return wstrError;
		}
		else
		{
			filesystem::create_directory(filesystem::path(ToString(wstrPath)));
		}
	}
	catch (filesystem::filesystem_error e)
	{
		wstrError = ToWstringError(e.what());
	}

	return wstrError;
}

//--------------------------------------

wstring CLogger::CreateFile(wstring& wstrPath)
{
	wstring wstrError = L"";

	try
	{
		tm tm = GetTime();

		wstring wstrFileName = FormatWText(LOG_FILE_NAME, tm.tm_year + SINCE_1900_YEAR, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

		wstrPath.append(NIX(L"/")WIN(L"\\") + wstrFileName);

		m_wofsLogFile.clear();
		m_wofsLogFile.open(ToString(wstrPath), ifstream::out);

		m_wofsLogFile.write(L"Start!\n", wstring(L"Start!\n").length());
	}
	catch (const wofstream::failure& e)
	{
		wstrError = ToWstringError(e.what());
	}

	return wstrError;
}
