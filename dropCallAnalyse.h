#ifndef __DROPCALLANALYSE_H__
#define __DROPCALLANALYSE_H__

#include <QString>
#include <QStringList>
#include <QMap>
#include <QVector>
#include <QPair>
#include "export_html.h"

struct RefInfo
{
	RefInfo()
	{
		nAverage = 0;
	}

	int nAverage;
	QVector<int> contVecByH;
};

struct CellInfo
{
	CellInfo()
		: longitude(0)
		, latitude(0)
		, angle(0)
	{

	}
	QString cellName;
	QString locationType;
	int angle;
	double longitude;
	double latitude;
};

struct LocationInfo
{
	LocationInfo()
		: longitude(361)
		, latitude(361)
	{

	}
	double longitude;
	double latitude;
};

class DropCallAnalyse : public QObject
{
public:
	DropCallAnalyse();
	void startAnalyse();

protected:
	void analyseSingleUser();
	void analyseCellStop();
	void analyseNeighborCell();
	void analyseJustShutDown();
	void analyseShutDownLongTime();
	void analyseOverCoverage();
	void calCellPNDist();
	void findNewCell();
	void sortCellsByConnectCnt();
	void getReferenceInfo();
	void getReferenceInfoFromResult(bool bConnectCountEmpty, bool bDropCountEmpty);
	void udpateReferenceInfo();
	void updateResultFile();
	void readNeighborCellInfo();
	void readCellInfo();
	void initShortID2LongIDMap();
	void analyseMMLChange();
	void exportHtml();
	void exportTopNHtml();
	void exportDropReasonHtml(int topN);
	LocationInfo getLoaction(const QString& shortID);
private:
	QString m_appDir;
	QString m_dataDir;
	QVector<QString> m_newCells;
	QVector<QString> m_topNCells;
	QMap<QString, CellInfo> m_cellInfo;
	QMap<QString, LocationInfo> m_locationInfo;
	QMap<QString, bool> m_userDropInfo;
	QMap<QString, CellConnectInfo> m_cellConnectInfo;
	QMap<QString, QStringList> m_neighborCells;
	QVector<QPair<QString, int> > m_cellSortedByConnectCnt;
	QMap<QString, QVector<RefInfo> > m_dropCntRefInfo;
	QMap<QString, QVector<RefInfo> > m_connectCntRefInfo;
	QMap<QString, QVector<QString> > m_dropReasonInfoByH;
	QMap<QString, QString> m_shortID2LongID;
	QMap<QString, AlarmInfo> m_alarmInfo;
	QMap<QString, DropReasonInfo> m_dropReasonInfo;
};

#endif //__DROPCALLANALYSE_H__