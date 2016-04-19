#include "querytool.h"
#include <QtWidgets/QApplication>
#include "qt_windows.h"
#include <QMessageBox>
#include "stdio.h"
#include <QDebug>
#include <QImage>


int exception_access_violation_filter(LPEXCEPTION_POINTERS p_exinfo)
{
	if (p_exinfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
	{
		qDebug() << QStringLiteral("存储保护异常\n");
		return 1;
	}
	else return 0;
}

int main(int argc, char *argv[])
{
	__try{
		_asm
		{
			xor eax, eax;
			mov[eax], 0;
		}
	}
	__except (exception_access_violation_filter(GetExceptionInformation()))
	{
		puts("外层的except块中");
	}

	QApplication a(argc, argv);
	QueryTool w;
	w.show();
	return a.exec();
}
