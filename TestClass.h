#pragma once
#include <sstream>
#include <iostream>
#include "Lib.h"

using namespace std;

class CTestClass
{
public:
	CTestClass();
	CTestClass(void* pTestingClass);
	~CTestClass();

	template <class TestFunc >
	void RunTest(TestFunc func, const string& test_name)
	{
		try {
			wprintf(L"\nStart unit test: %ls\n", ToWstring(test_name).c_str());
			func(m_pTestingClass);
			wprintf(L"End unit test: %ls\n\n", ToWstring(test_name).c_str());
		}
		catch (runtime_error & e) {
			m_iFailCount++;
			cerr << test_name << " fail: " << e.what() << endl;
		}
	}

private:
	int m_iFailCount;
	void* m_pTestingClass;
};

template <class T, class U>
void AssertEqual(const T& t, const U& u) {
	if (t != u)
	{
		wostringstream wos;
		wos << "Assertion failed: " << t << " != " << u << endl;
		wstring wstrText(wos.str());
		throw runtime_error(ToString(wstrText));
	}
}

template <class T, class U>
void AssertNotEqual(const T& t, const U& u) {
	if (t == u)
	{
		wostringstream wos;
		wos << "Assertion failed: " << t << " == " << u << endl;
		wstring wstrText(wos.str());
		throw runtime_error(ToString(wstrText));
	}
}

