#include "TestClass.h"


CTestClass::CTestClass()
{

}

CTestClass::CTestClass(void* pTestingClass)
{
	m_pTestingClass = pTestingClass;
	m_iFailCount = 0;
}


CTestClass::~CTestClass()
{
	if (m_iFailCount > 0)
	{
		cerr << m_iFailCount << " units are failed. Stop programm";
		getchar();
		exit(EXIT_FAILURE); 
	}
}
