#include "querytool.h"
#include <tlhelp32.h> 
#include <Psapi.h>
#include <QDebug>
#include <QMessageBox>
#include <QDir>
#define  QWS(x) QString::fromWCharArray(x)


QueryTool::QueryTool(QWidget *parent)
	: QMainWindow(parent)
{
	m_ui.setupUi(this);
	Init();
}

QueryTool::~QueryTool()
{

}

void QueryTool::Init()
{
	m_ui.processTable->setSelectionBehavior(QAbstractItemView::SelectItems);   //  设置成单选
	m_ui.btnProcessInfo->setVisible(false);
	m_ProcessSelectedRow = -1;
	Q_ASSERT(connect(m_ui.processTable, SIGNAL(cellClicked(int, int)), SLOT(onProcessTableClicked(int, int))));
}

void QueryTool::on_btnSelectProcess_clicked()
{
	m_ui.stackedWidget->setCurrentWidget(m_ui.pageSelectProcess);
	InsertProcessInfo();
	m_ui.btnProcessInfo->setVisible(true);
}

void QueryTool::on_btnProcessInfo_clicked()
{
	if (m_ProcessSelectedRow == -1)
	{
		return;
	}

	m_ui.stackedWidget->setCurrentWidget(m_ui.pageITAInfo);
	QTableWidgetItem *item  = m_ui.processTable->currentItem();
	int row = item->row();
	item = m_ui.processTable->item(row, 1);
	if (item==NULL)
	{
		QMessageBox::warning(nullptr, QStringLiteral("错误提示"), QStringLiteral("不存在的选项"));
		return;
	}
	DWORD PID = item->data(Qt::DisplayRole).toInt();
	wchar_t pathBuf[256] = { 0 };

	// 这个获取的是当前目录
	/*GetCurrentDirectory(256, pathBuf);*/

	// 获取进程目录即工作目录
	QString workPath = QCoreApplication::applicationDirPath();
	workPath = workPath + "/InjectDll.dll";
	Logger((workPath));
	// 注入
	InjectDll(workPath.toStdString().data(), PID);
}

void QueryTool::onProcessTableClicked(int row, int column)
{
	m_ProcessSelectedRow = row;
}

void QueryTool::InsertProcessInfo()
{
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof pe32;

	HANDLE  hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		printError("CreateToolhelp32Snapshot fail");
		return;
	}
	bool bMore = Process32First(hProcessSnap, &pe32);
	QTableWidget *pTableWidget = m_ui.processTable;

	int column = 0;
	pTableWidget->setColumnWidth(column++, 150);
	pTableWidget->setColumnWidth(column++, 60);
	pTableWidget->setColumnWidth(column++, 600);
	int row = 0;
	while (bMore)
	{
		QStringList list; 
		list << QStringLiteral("进程名称") << QStringLiteral("进程ID") << QStringLiteral("进程路径");
		pTableWidget->setHorizontalHeaderLabels(list);		
		pTableWidget->setItem(row, 0, new QTableWidgetItem(QString::fromWCharArray(pe32.szExeFile), 0));
		pTableWidget->setItem(row, 1, new QTableWidgetItem(QString::number(pe32.th32ProcessID), 0));
		pTableWidget->setItem(row, 2, new QTableWidgetItem(GetProcessPath(pe32.th32ProcessID), 0));
		row++;
		if (row == pTableWidget->rowCount())
		{
			pTableWidget->setRowCount(row + 10);
		}
		bMore = Process32Next(hProcessSnap, &pe32);
	}
	
}

void QueryTool::printError(const char* msg)
{
	DWORD eNum;
	TCHAR sysMsg[256];
	TCHAR* p;

	eNum = GetLastError();
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, eNum,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default lang.
		sysMsg, 256, NULL);

	// Trim the end of the line and terminate it with a null
	p = sysMsg;
	while ((*p > 31) || (*p == 9))
		++p;
	do { *p-- = 0; } while ((p >= sysMsg) &&
		((*p == '.') || (*p < 33)));

	// Display the message
	CHAR buf[255] = { 0 };
	
	sprintf(buf, "\n  WARNING: %s failed with error %d (%s)",
		msg, eNum, sysMsg);
	qDebug() << buf;
	QMessageBox::warning(nullptr, QStringLiteral("错误提示"), buf);
}

QString QueryTool::GetProcessPath(DWORD dwProcessId)
{
	TCHAR buf[255] = {0};
	HANDLE hProcess = NULL;
	hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, TRUE, dwProcessId);
	if (hProcess == NULL)
	{
		// 系统进程不能获取
		CloseHandle(hProcess);
		return QStringLiteral("系统进程");
	}
	int    nFilePathLen = 0;
	nFilePathLen = GetModuleFileNameEx(hProcess, 0, buf, 255);
	if (nFilePathLen == 0)
	{
		// 系统进程路径可能无法拿到
		CloseHandle(hProcess);
		return QStringLiteral("");
	}
	CloseHandle(hProcess);
	return QString::fromWCharArray(buf);
}

bool QueryTool::EnableDebugPriv(WCHAR * name)
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tp;
	LUID luid;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		return false;
	if (!LookupPrivilegeValue(NULL, name, &luid))
		return false;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	tp.Privileges[0].Luid = luid;
	if (!AdjustTokenPrivileges(hToken, 0, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
		return false;
	return true;
}
 
 bool QueryTool::InjectDll(const char * DllFullPath, DWORD dwRemoteProcessID)
 {
 	bool res = false;
 	HANDLE hRemoteProcess;
 	if (!EnableDebugPriv(SE_DEBUG_NAME))
 	{
		Logger(QStringLiteral("提升进程权限失败"));
 		return res;
 	}
 	if ((hRemoteProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwRemoteProcessID)) == NULL)
 	{
		Logger(QStringLiteral("打开目标进程失败"));
 		return res;
 	}
 	char * pszLibFileRemote;
 	pszLibFileRemote = (char *)VirtualAllocEx(hRemoteProcess, NULL, strlen(DllFullPath) + 1, MEM_COMMIT, PAGE_READWRITE);
 	if (pszLibFileRemote == NULL)
 	{
		Logger(QStringLiteral("目标进程内申请内存失败"));
 		return res;
 	}
 	DWORD dwWritten;
 	if (WriteProcessMemory(hRemoteProcess, pszLibFileRemote, (void *)DllFullPath, strlen(DllFullPath) + 1, &dwWritten) == 0)
 	{
		Logger(QStringLiteral("目标进程内改写内存失败"));
 		VirtualFreeEx(hRemoteProcess, pszLibFileRemote, strlen(DllFullPath) + 1, MEM_COMMIT);
 		CloseHandle(hRemoteProcess);
 		return res;
 	}
 	else
 	{
 		if (dwWritten != strlen(DllFullPath) + 1)
 		{
			Logger(QStringLiteral("目标进程内改写内存失败"));
 			VirtualFreeEx(hRemoteProcess, pszLibFileRemote, strlen(DllFullPath) + 1, MEM_COMMIT);
 			CloseHandle(hRemoteProcess);
 			return res;
 		}
 		else
			Logger(QStringLiteral("写入目标进程成功"));
 	}
 
 	LPVOID pFunc = LoadLibraryA;
 	DWORD dwID;
 	HANDLE hThread = CreateRemoteThread(hRemoteProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pFunc, pszLibFileRemote, 0, &dwID);
 	WaitForSingleObject(hThread, INFINITE);
 	VirtualFreeEx(hRemoteProcess, pszLibFileRemote, strlen(DllFullPath) + 1, MEM_COMMIT);
 	CloseHandle(hThread);
 	CloseHandle(hRemoteProcess);
	Logger(QStringLiteral("DLL加载到了目标进程"));
 	return res;
 }
 
 void QueryTool::Logger(QString& info)
 {
	 QMessageBox::warning(nullptr, QStringLiteral("错误提示"), info);
 }