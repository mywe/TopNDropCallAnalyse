
#include <QtCore/QCoreApplication>
#include "dropCallAnalyse.h"
#include <QApplication>
#include <QtGui>
#include <QWidget>
#include "export_html.h"
#include <QFile>
#include <QIODevice>
#include <QTextStream>

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	QTextCodec::setCodecForTr(QTextCodec::codecForName("system"));
	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("system"));
	QTextCodec::setCodecForLocale(QTextCodec::codecForName("system"));

	DropCallAnalyse dropCallAnalyse;
	dropCallAnalyse.startAnalyse();

	QTimer::singleShot(10 * 1000, &a, SLOT(quit()));

	return a.exec();
}
