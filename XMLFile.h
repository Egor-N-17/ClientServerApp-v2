#pragma once
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <boost/property_tree/xml_parser.hpp>
#include "Headers.h"
#include "Lib.h"

using namespace std;
using namespace boost;

class CXMLFile
{
public:
	static const wchar_t m_xmlNodeCmd[];
	static const wchar_t m_xmlNodeRes[];

public:
	CXMLFile();
	~CXMLFile();
	wstring Open(wstring wstrFileName);
	wstring GetVectorByKey(wstring wstrKey, vector<wstring>* wVector);

private:
	wstring Read(size_t iFileSize);
	wstring GetFilePath(wstring wstrFileName, wstring& wstrPath);
	wstring ReadKey(wstring wstrKey);

private:
	vector<wstring> m_vReadKey;
	ifstream m_ifsFile;
	property_tree::wptree m_wptXMLFile;
};