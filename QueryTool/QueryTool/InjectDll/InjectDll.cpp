// InjectDll.cpp : ���� DLL Ӧ�ó���ĵ���������
//

#include "stdafx.h"
#include "InjectDll.h"


// ���ǵ���������һ��ʾ��
INJECTDLL_API int nInjectDll=0;

// ���ǵ���������һ��ʾ����
INJECTDLL_API int fnInjectDll(void)
{
	return 42;
}

// �����ѵ�����Ĺ��캯����
// �й��ඨ�����Ϣ������� InjectDll.h
CInjectDll::CInjectDll()
{
	return;
}
