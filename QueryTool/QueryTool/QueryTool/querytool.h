#ifndef QUERYTOOL_H
#define QUERYTOOL_H

#include <QtWidgets/QMainWindow>
#include "ui_querytool.h"
#include "qt_windows.h"

class QueryTool : public QMainWindow
{
	Q_OBJECT

public:
	QueryTool(QWidget *parent = 0);
	~QueryTool();

protected:
	void Init();


private slots:
void on_btnSelectProcess_clicked();
void on_btnProcessInfo_clicked();
void onProcessTableClicked(int row, int column);


protected: 
	void InsertProcessInfo();
	void printError(const char* msg);
	QString GetProcessPath(unsigned long dwProcessId);
	bool EnableDebugPriv(WCHAR * name);

	bool InjectDll(const char * DllFullPath, DWORD dwRemoteProcessID);
	void Logger(QString& info);


private:
	Ui::QueryToolClass m_ui;
	int m_ProcessSelectedRow;
};

#endif // QUERYTOOL_H
