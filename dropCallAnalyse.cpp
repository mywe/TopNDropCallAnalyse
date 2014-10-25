#include "dropCallAnalyse.h"
#include <QtCore/QCoreApplication>
#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QIODevice>
#include <QTextStream>
#include <QPainterPath>
#include <QTimer>
#include <QTextCodec>
#define _USE_MATH_DEFINES
#include <math.h>
#include <QString>
#include <iostream>

const int idxOfCnntCountByD = 9;
const int idxOfCnntCountByH = 31;
const int dropReasonCount = 26;
const int TopNCount = 10;

double rad(double angle_d)
{
	return angle_d * M_PI / 180;
}

double getDistance(double lat1, double lng1, double lat2, double lng2)
{
	const double EARTH_RADIUS = 6378.137;
	double f = 1 / 298.257;
	double F = (lng1 + lng2) / 2;
	double G = (lng1 - lng2) / 2;
	double ramda = (lat1 - lat2) / 2;

	double S = std::pow(std::sin(rad(G)), 2) * (std::pow(std::cos(rad(ramda)), 2))
		+ std::pow(std::cos(rad(F)),2) * std::pow(std::sin(rad(ramda)), 2);
	double C = std::pow(std::cos(rad(G)),2) * std::pow(std::cos(rad(ramda)), 2)
		+ std::pow(std::sin(rad(F)), 2) * std::pow(std::sin(ramda), 2);

	double omega = std::atan(std::sqrt(S / C));
	double R = std::sqrt(S * C) / omega;
	double D = 2 * omega * EARTH_RADIUS;

	double H1 = (3 * R - 1) / (2 * C);
	double H2 = (3 * R + 1) / (2 * S);

	double res = D * (1 + f * H1 * (std::pow(std::sin(rad(F)), 2)) * (std::pow(std::cos(rad(G)), 2))
		- f * H2 * (std::pow(std::cos(rad(F)),2)) * (std::pow(std::sin(rad(G)), 2)));

	return res;
}


DropCallAnalyse::DropCallAnalyse()
{
	m_appDir = QCoreApplication::applicationDirPath();
	m_dataDir = m_appDir + "/result/";
}

void DropCallAnalyse::startAnalyse()
{
// 	QTextCodec::setCodecForTr(QTextCodec::codecForName("system"));
// 	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("system"));
// 	QTextCodec::setCodecForLocale(QTextCodec::codecForName("system"));

	QString dataFileName = QDateTime::currentDateTime().addDays(-1).toString("yyyyMMdd") + ".csv";
	QDir dataDir(m_dataDir);
	if (dataDir.exists(dataFileName))
	{
		analyseSingleUser();
		std::cout << "1" << std::endl;
		getReferenceInfo();
		std::cout << "2" << std::endl;
		readNeighborCellInfo();
		std::cout << "3" << std::endl;
		analyseCellStop();
		std::cout << "4" << std::endl;
		initShortID2LongIDMap();
		std::cout << "5" << std::endl;
		readCellInfo();
		std::cout << "6" << std::endl;
		analyseNeighborCell();
		std::cout << "7" << std::endl;
		findNewCell();
		std::cout << "8" << std::endl;
		calCellPNDist();
		std::cout << "9" << std::endl;
		analyseMMLChange();
		std::cout << "10" << std::endl;
		udpateReferenceInfo();
		std::cout << "11" << std::endl;
		analyseOverCoverage();
		std::cout << "12" << std::endl;
		updateResultFile();
		exportHtml();
		exportTopNHtml();
	}

	dataDir.remove(QDateTime::currentDateTime().addDays(-2).toString("yyyyMMdd") + "_NebAnalyse.txt");
	std::cout << "Analyse Finished!" << std::endl;

}

struct DropCountInfo
{
	DropCountInfo()
		: nConnect(0)
		, nDrop(0)
	{	}
	int nConnect;
	int nDrop;
	QMap<QString, int> imsiDropInfo;
};

void DropCallAnalyse::analyseSingleUser()
{
	QString nebCellFileName = m_dataDir
		+ QDateTime::currentDateTime().addDays(-1).toString("yyyyMMdd") + "_NebAnalyse.txt";
	QFile nebCellFile(nebCellFileName);

	if (!nebCellFile.open(QIODevice::ReadOnly))
		return;

	QTextStream readStream(&nebCellFile);
	QMap<QString, DropCountInfo> dropInfos;

	while (!readStream.atEnd())
	{
		QStringList imsiInfoList = readStream.readLine().split(',');

		QString carrierStr = imsiInfoList[4];
		DropCountInfo& dropCountInfo = dropInfos[carrierStr];
		int releaseVal = imsiInfoList[1].toInt();
		if (releaseVal >=3074 && releaseVal <= 3078)
		{
			QString imsiStr = imsiInfoList[0];
			if (dropCountInfo.imsiDropInfo.contains(imsiStr))
				++dropCountInfo.imsiDropInfo[imsiStr];
			else
				dropCountInfo.imsiDropInfo[imsiStr] = 1;
			++dropCountInfo.nDrop;
		}
		++dropCountInfo.nConnect;
	}

	nebCellFile.close();

	auto itDropInfo = dropInfos.begin();
	while (itDropInfo != dropInfos.end())
	{
		DropCountInfo& dropCountInfo = itDropInfo.value();
		const QMap<QString, int>& imsiDropInfoRef = dropCountInfo.imsiDropInfo;
		auto itIMSI = imsiDropInfoRef.begin();
		while (itIMSI != imsiDropInfoRef.end())
		{
			float fVal = itIMSI.value() / (float)dropCountInfo.nDrop;
			if (fVal > 0.5 || qFuzzyCompare(fVal, 0.5f))
			{
				m_userDropInfo[itDropInfo.key()] = true;

				QString dropReason("用户行为");
				DropReasonInfo& dropReasonInfoRef = m_dropReasonInfo[itDropInfo.key()];
				dropReasonInfoRef.dropReasonMap[dropReason].push_back("用户行为");
				break;
			}
			++itIMSI;
		}
		++itDropInfo;
	}
}

void DropCallAnalyse::analyseCellStop()
{
	QString dataFileName = QDateTime::currentDateTime().addDays(-1).toString("yyyyMMdd") + ".csv";
	dataFileName = m_dataDir + dataFileName;

	QFile dataFile(dataFileName);
	if (dataFile.open(QIODevice::ReadOnly))	
	{
		QTextStream readStream(&dataFile);
		readStream.readLine();

		QString curID;
		int topN = 0;
		while (!readStream.atEnd())
		{
			QStringList infoList = readStream.readLine().split(',');
			if (infoList[0] == "CDR掉话次数")
			{
				if (topN < TopNCount)
				{
					m_topNCells.push_back(infoList[1]);
					++topN;
				}

				curID = infoList[1];
				CellConnectInfo& cellConnectInfoRef = m_cellConnectInfo[curID];
				cellConnectInfoRef.cellName = infoList[2];
				cellConnectInfoRef.nDropCall = infoList[3].toInt();
				cellConnectInfoRef.nConnect = infoList[4].toInt();
				cellConnectInfoRef.fAvgEcio = infoList[7].toDouble();
				cellConnectInfoRef.fAvgDropDist = infoList[8].toDouble();
				for (int i = 0; i < 22; ++i)
					cellConnectInfoRef.dropCntByD[i] = infoList[i + idxOfCnntCountByD].toInt();
				for (int i = 0; i < 24; ++i)
					cellConnectInfoRef.dropCntByH[i] = infoList[i + idxOfCnntCountByH].toInt();
			}
			else if (curID == infoList[1] && infoList[0] == "呼叫次数")
			{
				CellConnectInfo& cellConnectInfoRef = m_cellConnectInfo[curID];
				for (int i = 0; i < 24; ++i)
					cellConnectInfoRef.connectByH[i] = infoList[i + idxOfCnntCountByH].toInt();
			}
			else if (curID == infoList[1] && infoList[0] == "掉话Ecio")
			{
				CellConnectInfo& cellConnectInfoRef = m_cellConnectInfo[curID];
				for (int i = 0; i < 22; ++i)
					cellConnectInfoRef.ecioByD[i] = infoList[i + idxOfCnntCountByD].toDouble();
			}
			else if (curID == infoList[1] && infoList[0] == "告警次数")
			{
				AlarmInfo& alarmInfoRef = m_alarmInfo[curID];
				for (int i = 0; i < 24; ++i)
				{
					if (infoList[i + idxOfCnntCountByH].size() < 1)
						continue;

					QStringList alarmInfoList = infoList[i + idxOfCnntCountByH].split(';');
					for (int idx = 0; idx < alarmInfoList.size(); ++idx)
					{
						QStringList singleAlarmList = alarmInfoList[idx].split(':');
						if (singleAlarmList.size() != 2)
							continue;
						alarmInfoRef.typeCountMap[singleAlarmList[0]].typeCount[i] = singleAlarmList[1].toInt();
					}
				}
			}
			else if (curID == infoList[1] && infoList[0] == "RSSI主集")
			{
				CellConnectInfo& cellConnectInfoRef = m_cellConnectInfo[curID];
				cellConnectInfoRef.fAvgOfMainRSSI = infoList[6].toDouble();
			}
			else if (curID == infoList[1] && infoList[0] == "RSSI分集")
			{
				CellConnectInfo& cellConnectInfoRef = m_cellConnectInfo[curID];
				cellConnectInfoRef.fAvgOfSubRSSI = infoList[6].toDouble();
			}
			else if (curID == infoList[1])
				continue;
			else
				break;
		}
	}

	dataFile.close();

	if (m_connectCntRefInfo.size())
	{
		sortCellsByConnectCnt();
		analyseJustShutDown();
		analyseShutDownLongTime();
	}
}

struct IMSIInfo
{
	QDateTime connectTime;
	QDateTime relTime;
	QString connectCarrier;
	QString relCarrier;
};

template<typename T>
void sortVec(QVector<T>& vec)
{
	int nCount= vec.size();
	for (int i = 0; i < nCount - 1; ++i)
	{
		T minVal = vec[i];
		for (int j = i + 1; j < nCount; ++j)
		{
			if (minVal > vec[j])
			{
				T tmpVal = minVal;
				minVal = vec[j];
				vec[j] = tmpVal;
			}
		}
		vec[i] = minVal;
	}
}

void DropCallAnalyse::analyseNeighborCell()
{
	QString nebCellFileName = m_dataDir
		+ QDateTime::currentDateTime().addDays(-1).toString("yyyyMMdd") + "_NebAnalyse.txt";
	QFile nebCellFile(nebCellFileName);

	if (!nebCellFile.open(QIODevice::ReadOnly))
		return;

	QTextStream readStream(&nebCellFile);
	QMap<QString, IMSIInfo> imsiInfos;
	QMap<QString, QVector<double> > dropDists;

	while (!readStream.atEnd())
	{
		QStringList imsiInfoList = readStream.readLine().split(',');

		int releaseVal = imsiInfoList[1].toInt();
		if (releaseVal >=3074 && releaseVal <= 3078)
		{
			IMSIInfo& imsiInfoRef = imsiInfos[imsiInfoList[0]];
			imsiInfoRef.connectTime = QDateTime::fromString(imsiInfoList[2], "yyyy.MM.dd hh:mm:ss");
			imsiInfoRef.relTime = QDateTime::fromString(imsiInfoList[3], "yyyy.MM.dd hh:mm:ss");
			imsiInfoRef.connectCarrier = imsiInfoList[4];
			imsiInfoRef.relCarrier = imsiInfoList[5];
			dropDists[imsiInfoList[5]].push_back(imsiInfoList[6].toDouble() * 30.5175);
		}

		if (imsiInfos.contains(imsiInfoList[0]))
		{
			const IMSIInfo& imsiInfoRef = imsiInfos[imsiInfoList[0]];

			if (m_userDropInfo.contains(imsiInfoRef.relCarrier))
				continue;

			if (imsiInfoRef.relTime.addSecs(60) < QDateTime::fromString(imsiInfoList[2], "yyyy.MM.dd hh:mm:ss"))
				continue;

			if (imsiInfoRef.relCarrier == imsiInfoList[4])
				continue;

			QString longID = m_shortID2LongID[imsiInfoRef.relCarrier];
			QStringList idList = m_neighborCells[longID];
			QString connectCarrier = m_shortID2LongID[imsiInfoList[4]];
			
			{
				if (longID.size() == 0 || connectCarrier.size() == 0)
					continue;

				QStringList longIDList = longID.split('_');
				QStringList connectCarrierList = connectCarrier.split('_');

				if (longIDList.size() < 4 || connectCarrierList.size() < 4)
					continue;

				if (longIDList[0] == connectCarrierList[0]
					&& longIDList[1] == connectCarrierList[1]
					&& longIDList[2] == connectCarrierList[2]
					&& longIDList[3] == connectCarrierList[3])
						continue;
			}

			int idx = -1;
			for (int i = 0; i < idList.size(); ++i)
			{
				if (idList[i] == connectCarrier)
				{
					idx = i + 1;
					break;
				}
			}

			if (idx > 30)
			{
				if (!m_dropReasonInfoByH.contains(longID))
					m_dropReasonInfoByH.insert(longID, QVector<QString>(dropReasonCount));

				QVector<QString>& dropReasonByH = m_dropReasonInfoByH[longID];
				
				QString info = connectCarrier + "_优先级不合理掉话";
				if (dropReasonByH[24].contains(info))
					continue;
				if (dropReasonByH[24].size() > 0)
					dropReasonByH[24] += ";";
				dropReasonByH[24] += info;

				QString dropReason("优先级不合理");
				DropReasonInfo& dropReasonInfoRef = m_dropReasonInfo[longID];
				dropReasonInfoRef.dropReasonMap[dropReason].push_back(connectCarrier + "-" + QString::number(idx));
			}
			else if (idx == -1)
			{
				if (!m_dropReasonInfoByH.contains(longID))
					m_dropReasonInfoByH.insert(longID, QVector<QString>(dropReasonCount));

				QVector<QString>& dropReasonByH = m_dropReasonInfoByH[longID];
				QString info = connectCarrier + "_邻区漏配";
				if (dropReasonByH[24].contains(info))
					continue;
				if (dropReasonByH[24].size() > 0)
					dropReasonByH[24] += ";";
				dropReasonByH[24] += info;

				QString dropReason("邻区漏配");
				DropReasonInfo& dropReasonInfoRef = m_dropReasonInfo[longID];
				dropReasonInfoRef.dropReasonMap[dropReason].push_back(connectCarrier);
			}
		}
	}

	nebCellFile.close();

	auto iterDropDist = dropDists.begin();
	while (iterDropDist != dropDists.end())
	{
		QVector<double> dropDistRef = iterDropDist.value();
		sortVec(dropDistRef);

		int index = dropDistRef.size() / 2;
		int nCount = 0;
		double totalVal = 0;
		for (; index < dropDistRef.size(); ++index)
		{
			nCount += 1;
			totalVal += dropDistRef[index];
		}

		QString longID = m_shortID2LongID[iterDropDist.key()];
		if (longID.size())
			m_cellConnectInfo[longID].fAvgDropDist = totalVal / nCount;
		++iterDropDist;
	}
}

void DropCallAnalyse::findNewCell()
{
	QDateTime dateTime = QDateTime::currentDateTime().addDays(-1);
	QString neborCellFileName = dateTime.addDays(-1).toString("yyyyMMdd") + "_NBRCDMACHS.txt";
	neborCellFileName = m_dataDir + neborCellFileName;

	QFile neborCellFile(neborCellFileName);
	if (!neborCellFile.open(QIODevice::ReadOnly))
		return;

	QVector<QString> preDayNeborCell;
	QTextStream readStream(&neborCellFile);

	while (!readStream.atEnd())
	{
		QStringList neborCells = readStream.readLine().split(':');
		if (preDayNeborCell.contains(neborCells[0]))
			continue;

		preDayNeborCell.push_back(neborCells[0]);
	}

	neborCellFile.close();

	auto iter = m_neighborCells.begin();
	while (iter != m_neighborCells.end())
	{
		if (!preDayNeborCell.contains(iter.key()) && !m_newCells.contains(iter.key()))
			m_newCells.push_back(iter.key());
		++iter;
	}

	std::cout<<"test begin\n";
	QFile newCellFile(m_dataDir + QString("新增载波列表.csv"));
	if (!newCellFile.open(QIODevice::ReadOnly))
	{
		newCellFile.open(QIODevice::WriteOnly);
		QTextStream stream(&newCellFile);
		stream << QObject::tr("载波,新开站日期,是否在工参找到经纬度") << "\n";
		newCellFile.close();
	}
	else
		newCellFile.close();
	std::cout<<"test finished\n";
	newCellFile.open(QIODevice::Append);
	QTextStream writeStream(&newCellFile);

	for (int i = 0; i < m_newCells.size(); ++i)
	{
		if (m_cellConnectInfo.contains(m_newCells[i]))
		{
			writeStream << m_newCells[i] << "," << dateTime.toString("yyyy.MM.dd") << ",";
			QStringList longIDStrList = m_newCells[i].split('_');
			QString shortID = longIDStrList[2] + "_" + longIDStrList[3] + "_" + longIDStrList[4];

			if (m_cellInfo.contains(shortID))
				writeStream << QObject::tr("是") << "\n";
			else
				writeStream << QObject::tr("否") << "\n";

			const QStringList& neighborCellList = m_neighborCells[m_newCells[i]];
			int nCount = qMin(neighborCellList.size(), 10);
			for (int i = 0; i < nCount; ++i)
			{
				const QString& neigborCell = neighborCellList[i];
				if (!m_cellConnectInfo.contains(neigborCell))
					continue;

				if (!m_dropReasonInfoByH.contains(neigborCell))
					m_dropReasonInfoByH.insert(neigborCell, QVector<QString>(dropReasonCount));

				QVector<QString>& dropReasonByH = m_dropReasonInfoByH[neigborCell];
				if (dropReasonByH[24].size() > 0)
					dropReasonByH[24] += ";";
				QString dropReasonDetail = "可能受新增载波" + m_newCells[i] + "影响_优先级" + QString::number(i + 1);
				dropReasonByH[24] += dropReasonDetail;

				QString dropReason("新增载波");
				DropReasonInfo& dropReasonInfoRef = m_dropReasonInfo[neigborCell];
				dropReasonInfoRef.dropReasonMap[dropReason].push_back(dropReasonDetail);
			}
		}
	}

	newCellFile.close();
}

void DropCallAnalyse::sortCellsByConnectCnt()
{
	auto itOfConnectInfo = m_cellConnectInfo.begin();
	while (itOfConnectInfo != m_cellConnectInfo.end())
	{
		auto itOfSortedVec = m_cellSortedByConnectCnt.begin();
		for (; itOfSortedVec != m_cellSortedByConnectCnt.end(); ++itOfSortedVec)
		{
			if (itOfConnectInfo.value().nConnect >= itOfSortedVec->second)
				break;
		}
		m_cellSortedByConnectCnt.insert(itOfSortedVec,
										QPair<QString, int>(itOfConnectInfo.key(),
															itOfConnectInfo.value().nConnect));
		++itOfConnectInfo;
	}
}

void calConnectCntAvgValue(QMap<QString, QVector<RefInfo> >& refInfos)
{
	auto itRefInfo = refInfos.begin();
	while (itRefInfo != refInfos.end())
	{
		QVector<RefInfo>& refInfo = itRefInfo.value();
		int vecSize = refInfo.size();
		for (int i = 0; i < vecSize; ++i)
		{
			QVector<int> cntVec = refInfo[i].contVecByH;
			sortVec(cntVec);

			int startIdx = 0, endIdx = cntVec.size();
			if (endIdx > 2)
			{
				++startIdx;
				--endIdx;
			}

			int totalVal = 0;
			for (int j = startIdx; j < endIdx; ++j)
				totalVal += cntVec[j];

			refInfo[i].nAverage = totalVal / (endIdx - startIdx);
		}

		++itRefInfo;
	}
}

void calDropCntStanderValue(QMap<QString, QVector<RefInfo> >& refInfos)
{
	auto itRefInfo = refInfos.begin();
	while (itRefInfo != refInfos.end())
	{
		QVector<RefInfo>& refInfo = itRefInfo.value();
		int vecSize = refInfo.size();
		for (int i = 0; i < vecSize; ++i)
		{
			QVector<int> cntVec = refInfo[i].contVecByH;
			sortVec(cntVec);

			int targetIdx = 9;
			if (cntVec.size() < 10)
				targetIdx = cntVec.size() / 2;

			refInfo[i].nAverage = cntVec[targetIdx];
		}
		++itRefInfo;
	}
}

void readRefInfo(QVector<RefInfo>& refInfoVec, QStringList& infoList)
{
	for (int i = 0; i < 24; ++i)
		refInfoVec[i].contVecByH.push_back(infoList[i + 1].toInt());
}

void DropCallAnalyse::getReferenceInfo()
{
	QString connectCntFileName = m_dataDir + "connectCountFile.txt";
	QFile connectCntFile(connectCntFileName);
	if (connectCntFile.open(QIODevice::ReadOnly))
	{
		QTextStream readStream(&connectCntFile);
		QString curCell;

		while (!readStream.atEnd())
		{
			QStringList infoList = readStream.readLine().split(',');
			if (infoList[0] != curCell)
			{
				curCell = infoList[0];
				m_connectCntRefInfo.insert(curCell, QVector<RefInfo>(24));
				readRefInfo(m_connectCntRefInfo[curCell], infoList);
			}
			else if (infoList[0] == curCell)
			{
				readRefInfo(m_connectCntRefInfo[curCell], infoList);
			}
		}

		connectCntFile.close();
	}

	QString dropCntFileName = m_dataDir + "dropCountFile.txt";
	QFile dropCntFile(dropCntFileName);
	if (dropCntFile.open(QIODevice::ReadOnly))
	{
		QTextStream readStream(&dropCntFile);
		QString curCell;

		while (!readStream.atEnd())
		{
			QStringList infoList = readStream.readLine().split(',');
			if (infoList[0] != curCell)
			{
				curCell = infoList[0];
				m_dropCntRefInfo.insert(curCell, QVector<RefInfo>(24));
				readRefInfo(m_dropCntRefInfo[curCell], infoList);
			}
			else if (infoList[0] == curCell)
			{
				readRefInfo(m_dropCntRefInfo[curCell], infoList);
			}
		}

		dropCntFile.close();
	}

	bool bConnectCountEmpty = (m_connectCntRefInfo.size() == 0);
	bool bDropCountEmpty = (m_dropCntRefInfo.size() == 0);

	if (bConnectCountEmpty || bDropCountEmpty)
		getReferenceInfoFromResult(bConnectCountEmpty, bDropCountEmpty);

	calConnectCntAvgValue(m_connectCntRefInfo);
	calDropCntStanderValue(m_dropCntRefInfo);
}

void DropCallAnalyse::getReferenceInfoFromResult(bool bConnectCountEmpty, bool bDropCountEmpty)
{
	QDateTime startTime = QDateTime::currentDateTime().addDays(-16);

	for (int i = 0; i < 15; ++i)
	{
		QString dataFileName = m_dataDir + startTime.addDays(i).toString("yyyyMMdd") + ".csv";
		QFile dataFile(dataFileName);
		if (!dataFile.open(QIODevice::ReadOnly))
			continue;

		QTextStream readStream(&dataFile);
		readStream.readLine();

		while (!readStream.atEnd())
		{
			QStringList infoList = readStream.readLine().split(',');

			if (infoList[0] == "CDR掉话次数" && bDropCountEmpty)
			{
				QString curCell = infoList[1];
				if (!m_dropCntRefInfo.contains(curCell))
					m_dropCntRefInfo.insert(curCell, QVector<RefInfo>(24));

				QVector<RefInfo>& refInfoRef = m_dropCntRefInfo[curCell];
				for (int h = 0; h < 24; ++h)
					refInfoRef[h].contVecByH.push_back(infoList[idxOfCnntCountByH + h].toInt());
			}
			else if (infoList[0] == "呼叫次数" && bConnectCountEmpty)
			{
				QString curCell = infoList[1];
				if (!m_connectCntRefInfo.contains(curCell))
					m_connectCntRefInfo.insert(curCell, QVector<RefInfo>(24));

				QVector<RefInfo>& refInfoRef = m_connectCntRefInfo[curCell];
				for (int h = 0; h < 24; ++h)
					refInfoRef[h].contVecByH.push_back(infoList[idxOfCnntCountByH + h].toInt());
			}
		}

		dataFile.close();
	}
}

void DropCallAnalyse::udpateReferenceInfo()
{
	auto itConnectInfo = m_cellConnectInfo.begin();
	while (itConnectInfo != m_cellConnectInfo.end())
	{
		QString curCell = itConnectInfo.key();
		if (!m_connectCntRefInfo.contains(curCell))
			m_connectCntRefInfo.insert(curCell, QVector<RefInfo>(24));

		if (!m_dropCntRefInfo.contains(curCell))
			m_dropCntRefInfo.insert(curCell, QVector<RefInfo>(24));

		QVector<RefInfo>& connectCntRefInfo = m_connectCntRefInfo[curCell];
		if (connectCntRefInfo.size() < 24)
			connectCntRefInfo.resize(24);

		QVector<RefInfo>& dropCntRefInfo = m_dropCntRefInfo[curCell];
		if (dropCntRefInfo.size() < 24)
			dropCntRefInfo.resize(24);

		const CellConnectInfo& cellConnectInfoRef = itConnectInfo.value();

		for (int i = 0; i < 24; ++i)
		{
			if (connectCntRefInfo[i].contVecByH.size() >= 15)
				connectCntRefInfo[i].contVecByH.pop_front();
			connectCntRefInfo[i].contVecByH.push_back(cellConnectInfoRef.connectByH[i]);

			if (dropCntRefInfo[i].contVecByH.size() >= 15)
				dropCntRefInfo[i].contVecByH.pop_front();
			dropCntRefInfo[i].contVecByH.push_back(cellConnectInfoRef.dropCntByH[i]);
		}
		++itConnectInfo;
	}

	QString connectCntFileName = m_dataDir + "connectCountFile.txt";
	QFile connectCntFile(connectCntFileName);
	connectCntFile.open(QIODevice::WriteOnly);
	QTextStream connectCntWriteStream(&connectCntFile);

	auto itConnectCnt = m_connectCntRefInfo.begin();
	while (itConnectCnt != m_connectCntRefInfo.end())
	{
		if (itConnectCnt.value().size() == 24)
		{
			int nDay = (itConnectCnt.value())[0].contVecByH.size();

			for (int i = 0; i < nDay; ++i)
			{
				connectCntWriteStream << itConnectCnt.key();
				for (int j = 0; j < 24; ++j)
				{
					connectCntWriteStream << "," << (itConnectCnt.value())[j].contVecByH[i];
				}
				connectCntWriteStream << "\n";
			}
		}
		++itConnectCnt;
	}

	connectCntFile.close();

	QString dropCntFileName = m_dataDir + "dropCountFile.txt";
	QFile dropCntFile(dropCntFileName);
	dropCntFile.open(QIODevice::WriteOnly);
	QTextStream dropCntWriteStream(&dropCntFile);

	auto itDropCnt = m_dropCntRefInfo.begin();
	while (itDropCnt != m_dropCntRefInfo.end())
	{
		if (itDropCnt.value().size() == 24)
		{
			int nDay = (itDropCnt.value())[0].contVecByH.size();

			for (int i = 0; i < nDay; ++i)
			{
				dropCntWriteStream << itDropCnt.key();
				for (int j = 0; j < 24; ++j)
				{
					dropCntWriteStream << "," << (itDropCnt.value())[j].contVecByH[i];
				}
				dropCntWriteStream << "\n";
			}
		}
		
		++itDropCnt;
	}
}

void DropCallAnalyse::readNeighborCellInfo()
{
	QString neborCellFileName = QDateTime::currentDateTime().addDays(-1).toString("yyyyMMdd") + "_NBRCDMACHS.txt";
	QFile neborCellFile(m_dataDir + neborCellFileName);
	neborCellFile.open(QIODevice::ReadOnly);
	QTextStream readStream(&neborCellFile);

	while (!readStream.atEnd())
	{
		QStringList infoList = readStream.readLine().split(':');
		m_neighborCells[infoList[0]] = infoList[1].split(','); 
	}

	neborCellFile.close();
}

void DropCallAnalyse::analyseJustShutDown()
{
	int eightyPercentIdx = m_cellSortedByConnectCnt.size() * 0.8;
	for (int i = 0; i < eightyPercentIdx; ++i)
	{
		QString curCellName = m_cellSortedByConnectCnt[i].first;

		if (m_userDropInfo.contains(curCellName))
			continue;

		const CellConnectInfo& cellConnectInfoRef = m_cellConnectInfo[curCellName];
		const QVector<RefInfo>& connectCntInfoRef = m_connectCntRefInfo[curCellName];
		if (connectCntInfoRef.size() != 24)
			continue;

		for (int h = 9; h < 23; ++h)
		{
			if (cellConnectInfoRef.connectByH[h] != 0)
				continue;
			if (connectCntInfoRef[h].nAverage == 0)
				continue;

			const QStringList& neighborCellsRef = m_neighborCells[curCellName];
			int nCount = qMin(neighborCellsRef.size(), 10);

			for (int pri = 0; pri < nCount; ++pri)
			{
				if (!m_dropReasonInfoByH.contains(neighborCellsRef[pri]))
					m_dropReasonInfoByH.insert(neighborCellsRef[pri], QVector<QString>(dropReasonCount));

				QVector<QString>& dropReasonByH = m_dropReasonInfoByH[neighborCellsRef[pri]];
				if (dropReasonByH[h].size() > 0)
					dropReasonByH[h] += ";";
				dropReasonByH[h] += (curCellName + "_优先级" + QString::number(pri) + "_0话务");

				QString dropReason("载波关停");
				DropReasonInfo& dropReasonInfoRef = m_dropReasonInfo[neighborCellsRef[pri]];
				dropReasonInfoRef.dropReasonMap[dropReason].push_back(curCellName + "-优先级:" + QString::number(pri) + "-0话务");
			}

			double val = (double)cellConnectInfoRef.dropCntByH[h] / m_dropCntRefInfo[curCellName][h].nAverage;
			if (val < 2)
				continue;

			if (!m_dropReasonInfoByH.contains(curCellName))
				m_dropReasonInfoByH.insert(curCellName, QVector<QString>(dropReasonCount));

			QVector<QString>& dropReasonByH = m_dropReasonInfoByH[curCellName];
			if (dropReasonByH[h].size() > 0)
				dropReasonByH[h] += ";";
			dropReasonByH[h] += ("恶化_比值为" + QString::number(val));
		}

		qreal totalEcio = 0;
		for (int i = 0; i < 22; ++i)
			totalEcio += cellConnectInfoRef.ecioByD[i];
		qreal avgEcio = totalEcio / 22;

		if (avgEcio < -14)
		{
			int maxEcioIdx = 0;
			int maxDropCntIdx = 0;
			for (int i = 0; i < 22; ++i)
			{
				if (cellConnectInfoRef.ecioByD[i] > cellConnectInfoRef.ecioByD[maxEcioIdx])
					maxEcioIdx = i;

				if (cellConnectInfoRef.dropCntByD[i] > cellConnectInfoRef.dropCntByD[maxDropCntIdx])
					maxDropCntIdx = i;
			}

			if (maxEcioIdx == maxDropCntIdx)
			{
				if (!m_dropReasonInfoByH.contains(curCellName))
					m_dropReasonInfoByH.insert(curCellName, QVector<QString>(dropReasonCount));

				QVector<QString>& dropReasonByH = m_dropReasonInfoByH[curCellName];
				if (dropReasonByH[24].size() > 0)
					dropReasonByH[24] += ";";
				dropReasonByH[24] += "近距离弱覆盖_可能输出功率不足_站点过低或者严重阻挡";

				QString dropReason("弱覆盖");
				DropReasonInfo& dropReasonInfoRef = m_dropReasonInfo[curCellName];
				dropReasonInfoRef.dropReasonMap[dropReason].push_back("近距离弱覆盖");
			}
		}
	}
}

void DropCallAnalyse::analyseShutDownLongTime()
{
	auto itConnectInfo = m_cellConnectInfo.begin();
	QFile longTimeShutDownFile(m_dataDir + "longTimeShutDownFile.txt");
	QMap<QString, QStringList> longTimeShutDownInfo;
	if (longTimeShutDownFile.open(QIODevice::ReadOnly))
	{
		QFile tmpFile(m_dataDir + "longTimeShutDownFile1.txt");
		tmpFile.open(QIODevice::WriteOnly);
		QTextStream writeStream(&tmpFile);

		QTextStream readStream(&longTimeShutDownFile);
		while (!readStream.atEnd())
		{
			QString line(readStream.readLine());
			QStringList lineList(line.split(','));
			if (lineList[2].size())
				longTimeShutDownInfo[lineList[0]] = lineList;
			else
				writeStream << line << "\n";
		}
		tmpFile.close();
	}
	longTimeShutDownFile.close();

	QMap<QString, int> avgConnectCount;
	while (itConnectInfo != m_cellConnectInfo.end())
	{
		const QString& cellID = itConnectInfo.key();
		if (!m_connectCntRefInfo.contains(cellID))
		{
			++itConnectInfo;
			continue;
		}

		const QVector<RefInfo>& refInfoVecRef = m_connectCntRefInfo[cellID];
		if (refInfoVecRef.size() == 0)
		{
			++itConnectInfo;
			continue;
		}

		int nDay = refInfoVecRef.front().contVecByH.size();
		int totalVal = 0;
		for (int h = 0; h < 24; ++h)
		{
			const RefInfo& refInfoRef = refInfoVecRef[h];
			for (int d = 0; d < nDay; ++d)
			{
				totalVal += refInfoRef.contVecByH[d];
			}
		}

		avgConnectCount[cellID] = totalVal / nDay;
		++itConnectInfo;
	}

	itConnectInfo = m_cellConnectInfo.begin();
	while (itConnectInfo != m_cellConnectInfo.end())
	{
		const QString& cellID = itConnectInfo.key();
		int nConnect = itConnectInfo.value().nConnect;
		if (nConnect == 0
			&& (!avgConnectCount.contains(cellID) || avgConnectCount[cellID] != 0))
		{
			QString curCellName = itConnectInfo.key();
			const QStringList& neighborCellsRef = m_neighborCells[curCellName];
			int nCount = qMin(neighborCellsRef.size(), 10);

			QStringList info;
			info << curCellName << QDateTime::currentDateTime().addDays(-1).toString("yyyyMMdd") << "";

			for (int pri = 0; pri < nCount; ++pri)
			{
				info << neighborCellsRef[pri];

				if (!m_dropReasonInfoByH.contains(neighborCellsRef[pri]))
					m_dropReasonInfoByH.insert(neighborCellsRef[pri], QVector<QString>(dropReasonCount));

				QVector<QString>& dropReasonByH = m_dropReasonInfoByH[neighborCellsRef[pri]];
				if (dropReasonByH[24].size() > 0)
					dropReasonByH[24] += ";";
				dropReasonByH[24] += "受_" + curCellName + "_站长期关停影响_优先级" + QString::number(pri);

				QString dropReason("载波长期关停");
				DropReasonInfo& dropReasonInfoRef = m_dropReasonInfo[neighborCellsRef[pri]];
				dropReasonInfoRef.dropReasonMap[dropReason].push_back("受-" + curCellName + "-站长期关停影响-优先级:" + QString::number(pri));
			}

			if (!longTimeShutDownInfo.contains(curCellName))
				longTimeShutDownInfo[curCellName] = info;
		}
		else if (nConnect > 0 && longTimeShutDownInfo.contains(cellID))
		{
			QStringList& shutDownInfoRef = longTimeShutDownInfo[cellID];
			const QStringList& neighborCells = m_neighborCells[cellID];

			int nSame = 0;
			int nCount = qMin(shutDownInfoRef.size() - 3, 10);
			for (int i = 0; i < nCount; ++i)
			{
				int nSize = qMin(neighborCells.size(), 10);
				for (int j = 0; j < nSize; ++j)
				{
					if (shutDownInfoRef[i + 3] == neighborCells[j])
						++nSame;
				}
			}

			if (nSame > 4)
			{
				shutDownInfoRef[2] = QDateTime::currentDateTime().addDays(-1).toString("yyyyMMdd");
				int nSize = qMin(neighborCells.size(), 10);
				for (int i = 0; i < nSize; ++i)
				{
					if (!m_dropReasonInfoByH.contains(neighborCells[i]))
						m_dropReasonInfoByH.insert(neighborCells[i], QVector<QString>(dropReasonCount));

					QVector<QString>& dropReasonByH = m_dropReasonInfoByH[neighborCells[i]];
					if (dropReasonByH[24].size() > 0)
						dropReasonByH[24] += ";";
					dropReasonByH[24] += "受_" + cellID + "_站恢复_优先级" + QString::number(i);

					QString dropReason("载波恢复");
					DropReasonInfo& dropReasonInfoRef = m_dropReasonInfo[neighborCells[i]];
					dropReasonInfoRef.dropReasonMap[dropReason].push_back("受-" + cellID + "-站恢复-优先级:" + QString::number(i));
				}
			}
		}

		++itConnectInfo;
	}

	auto itShutDown = longTimeShutDownInfo.begin();
	QFile tmpFile(m_dataDir + "longTimeShutDownFile1.txt");
	tmpFile.open(QIODevice::Append);
	QTextStream writeStream(&tmpFile);

	while (itShutDown != longTimeShutDownInfo.end())
	{
		QStringList shutDownList = itShutDown.value();
		writeStream << shutDownList[0];
		for (int i = 1; i < shutDownList.size(); ++i)
			writeStream << "," << shutDownList[i];
		writeStream << "\n";

		++itShutDown;
	}
	tmpFile.close();

	longTimeShutDownFile.remove();
	tmpFile.rename(m_dataDir + "longTimeShutDownFile.txt");
}

void DropCallAnalyse::analyseOverCoverage()
{
	int test =0 ;
	auto itCell = m_cellConnectInfo.begin();
	while (itCell != m_cellConnectInfo.end())
	{
		if (m_userDropInfo.contains(itCell.key()))
		{
			++itCell;
			continue;
		}

		CellConnectInfo& cellCnntInfoRef = itCell.value();
		QStringList longID = itCell.key().split('_');
		QString shortID = longID[2] + "_" + longID[3] + "_" + longID[4];
		++test;

		CellInfo& cellInfoRef = m_cellInfo[shortID];
		QPointF ptCenter(cellInfoRef.longitude, cellInfoRef.latitude);
		qreal radiu = itCell.value().fAvgDropDist / 50 * 1.62;

		int startAngle = 0;
		int angle = cellInfoRef.angle % 360;
		switch (angle / 90)
		{
		case 0:
			startAngle = 90 - angle;
			break;
		case 1:
			startAngle = 360 + (angle - 90);
			break;
		case 2:
			startAngle = 180 + (270 - angle);
			break;
		case 3:
			startAngle = 90 + (360 - angle);
			break;
		}

		startAngle = (startAngle + 330) % 360;

		QPainterPath path;
		path.moveTo(0, 0);
		path.arcTo(-radiu / 2, -radiu / 2, radiu, radiu, startAngle, 60);
		path.closeSubpath();

		int containCount = 0;
		auto it = m_cellInfo.begin();
		while (it != m_cellInfo.end())
		{
			CellInfo& cellRef = it.value();
			if (cellRef.locationType == "室外" && shortID != it.key())
			{
				qreal x = (cellRef.longitude - ptCenter.x()) * 3600;
				qreal y = (cellRef.latitude - ptCenter.y()) * 3600;
				if (path.contains(QPointF(x, y)))
					containCount += 1;
			}
			++it;
		}

		if (containCount > 5)
		{
			QString cellID = itCell.key();
			if (!m_dropReasonInfoByH.contains(cellID))
				m_dropReasonInfoByH.insert(cellID, QVector<QString>(dropReasonCount));

			QVector<QString>& dropReasonByH = m_dropReasonInfoByH[cellID];
			if (dropReasonByH[24].size() > 0)
				dropReasonByH[24] += ";";
			dropReasonByH[24] += "越区覆盖";
			if (cellCnntInfoRef.fAvgEcio > -10)
				dropReasonByH[24] += "（可能直放站引起）";

			QString dropReason("越区覆盖");
			DropReasonInfo& dropReasonInfoRef = m_dropReasonInfo[cellID];
			dropReasonInfoRef.dropReasonMap[dropReason].push_back("越区覆盖");
		}

		++itCell;
	}
}

QString transLongIDToShortID(const QString& longID)
{
	QStringList longIDList = longID.split('_');
	return longIDList[2] + "_" + longIDList[3] + "_" + longIDList[4];
}

void DropCallAnalyse::calCellPNDist()
{
	QFile pnFile(m_dataDir + QDateTime::currentDateTime().addDays(-1).toString("yyyyMMdd") + "_PN.txt");
	if (!pnFile.open(QIODevice::ReadOnly))
		return;

	QTextStream readStream(&pnFile);
	QMap<QString, int> newCellID2PN;
	QMap<int, QVector<QString> > PN2CellIDs;
	QMap<QString, int> cellID2PN;

	while (!readStream.atEnd())
	{
		QStringList cellID2PnList = readStream.readLine().split(':');
		if (m_newCells.contains(cellID2PnList[0]))
			newCellID2PN[cellID2PnList[0]] = cellID2PnList[1].toInt();
		else
		{
			cellID2PN[cellID2PnList[0]] = cellID2PnList[1].toInt();
			PN2CellIDs[cellID2PnList[1].toInt()].push_back(cellID2PnList[0]);
		}
	}

	auto itNewCell = newCellID2PN.begin();
	while (itNewCell != newCellID2PN.end())
	{
		QString newCellShortID = transLongIDToShortID(itNewCell.key());
		LocationInfo newCellLoaction = getLoaction(newCellShortID);
		if (qFuzzyCompare(newCellLoaction.latitude, 361.0)
			|| qFuzzyCompare(newCellLoaction.longitude, 361.0))
		{
			++itNewCell;
			continue;
		}

		const QVector<QString>& cellIDsRef = PN2CellIDs[itNewCell.value()];
		double minDist = 6378.137;
		QString nearestCellName;
		for (int i = 0; i < cellIDsRef.size(); ++i)
		{
			LocationInfo nearCellLocation = getLoaction(transLongIDToShortID(cellIDsRef[i]));
			if (qFuzzyCompare(nearCellLocation.latitude, 361.0)
				|| qFuzzyCompare(nearCellLocation.longitude, 361.0))
				continue;

			double dist = getDistance(newCellLoaction.latitude, newCellLoaction.longitude,
										nearCellLocation.latitude, nearCellLocation.longitude);
			if (minDist > dist)
			{
				minDist = dist;
				nearestCellName = cellIDsRef[i];
			}
		}
		if (nearestCellName.size())
		{
			QString newCellLongID = itNewCell.key();

			if (!m_dropReasonInfoByH.contains(newCellLongID))
				m_dropReasonInfoByH.insert(newCellLongID, QVector<QString>(dropReasonCount));

			QVector<QString>& dropReasonByH = m_dropReasonInfoByH[newCellLongID];
			dropReasonByH[25] = nearestCellName + ":" + QString::number(qMin(10.0, minDist)) + "km";
		}
		++itNewCell;
	}

	auto itCell = cellID2PN.begin();
	while (itCell != cellID2PN.end())
	{
		QString cellShortID = transLongIDToShortID(itCell.key());
		LocationInfo curCellLoaction = getLoaction(cellShortID);

		if (qFuzzyCompare(curCellLoaction.longitude, 361.0)
			|| qFuzzyCompare(curCellLoaction.latitude, 361.0))
		{
			++itCell;
			continue;
		}

		const QVector<QString>& cellIDsRef = PN2CellIDs[itCell.value()];
		double minDist = 6378.137;
		QString nearestCellName;
		for (int i = 0; i < cellIDsRef.size(); ++i)
		{
			if (itCell.key() == cellIDsRef[i])
				continue;

			LocationInfo nearCellLocation = getLoaction(transLongIDToShortID(cellIDsRef[i]));
			
			if (qFuzzyCompare(nearCellLocation.longitude, 361.0)
				|| qFuzzyCompare(nearCellLocation.latitude, 361.0))
				continue;

			double dist = getDistance(curCellLoaction.latitude, curCellLoaction.longitude,
										nearCellLocation.latitude, nearCellLocation.longitude);
			if (minDist > dist)
			{
				minDist = dist;
				nearestCellName = cellIDsRef[i];
			}
		}
		if (nearestCellName.size())
		{
			QString cellLongID = itCell.key();

			if (!m_dropReasonInfoByH.contains(cellLongID))
				m_dropReasonInfoByH.insert(cellLongID, QVector<QString>(dropReasonCount));

			QVector<QString>& dropReasonByH = m_dropReasonInfoByH[cellLongID];
			dropReasonByH[25] = nearestCellName + ":" + QString::number(qMin(10.0, minDist)) + "km";
		}
		++itCell;
	}
}

LocationInfo DropCallAnalyse::getLoaction(const QString& shortID)
{
	LocationInfo location;
	QStringList shortIDList = shortID.split('_');
	QString cellAndSectorID = shortIDList[0] + "_" + shortIDList[1];

	if (m_cellInfo.contains(shortID))
	{
		const CellInfo& cellInfoRef = m_cellInfo[shortID];
		location.latitude = cellInfoRef.latitude;
		location.longitude = cellInfoRef.longitude;
	}
	else if (m_locationInfo.contains(cellAndSectorID))
	{
		location = m_locationInfo[cellAndSectorID];
	}

	return location;
}

void DropCallAnalyse::updateResultFile()
{
	QString dataFileName = QDateTime::currentDateTime().addDays(-1).toString("yyyyMMdd") + ".csv";
	dataFileName = m_dataDir + dataFileName;

	QString resultFileName = m_dataDir + QDateTime::currentDateTime().addDays(-1).toString("yyyyMMdd") + "_1.csv";

	QFile dataFile(dataFileName);
	dataFile.open(QIODevice::ReadOnly);

	QFile resultFile(resultFileName);
	resultFile.open(QIODevice::WriteOnly);

	{
		QTextStream textStream(&dataFile);
		QTextStream writeStream(&resultFile);

		writeStream << textStream.readLine() << "\n";

		QString curID;
		while (!textStream.atEnd())
		{
			QString line = textStream.readLine();
			writeStream << line << "\n";

			QStringList infoList = line.split(',');
			if (infoList[0] == "CDR掉话次数")
			{
				curID = infoList[1];
			}
			else if (curID == infoList[1] && infoList[0] == "RSSI分集")
			{
				if (!m_dropReasonInfoByH.contains(curID))
					continue;

				const QVector<QString>& dropReasonInfoRef = m_dropReasonInfoByH[curID];
				writeStream << QObject::tr("掉话原因,") << curID << "," << dropReasonInfoRef[25];
				for (int i = 0; i < 5; ++i)
					writeStream << ",";

				for (int i = 0; i < 23; ++i)
					writeStream << ",";

				for (int i = 0; i < 24; ++i)
					writeStream << "," << dropReasonInfoRef[i];

				writeStream << "," << dropReasonInfoRef[24] << "\n";
			}
			else if (curID == infoList[1])
				continue;
			else
				break;
		}

		while (!textStream.atEnd())
		{
			writeStream << textStream.readLine() << "\n";
		}
	}

	dataFile.close();
	resultFile.close();

	dataFile.remove();
	resultFile.rename(m_dataDir + QDateTime::currentDateTime().addDays(-1).toString("yyyyMMdd") + ".csv");
}

void DropCallAnalyse::readCellInfo()
{
	QFile cellInfoFile(m_appDir + "/settingFile/cellInfo.csv");//最好放在setting中
	if (!cellInfoFile.open(QIODevice::ReadOnly))
		return;

	QTextStream readStream(&cellInfoFile);
	readStream.readLine();
	
	while (!readStream.atEnd())
	{
		QStringList cellInfoList = readStream.readLine().split(',');
		QString shortId = cellInfoList[2] + "_" + cellInfoList[3] + "_" +cellInfoList[4];
		CellInfo& cellInfoRef = m_cellInfo[shortId];
		cellInfoRef.cellName = cellInfoList[5];
		cellInfoRef.longitude = cellInfoList[6].toDouble();
		cellInfoRef.latitude = cellInfoList[7].toDouble();
		cellInfoRef.angle = cellInfoList[8].toInt();
		cellInfoRef.locationType = cellInfoList[9];

		QString cellAndSectorID = cellInfoList[2] + "_" + cellInfoList[3];
		if (!m_locationInfo.contains(cellAndSectorID))
		{
			m_locationInfo[cellAndSectorID].longitude = cellInfoList[6].toDouble();
			m_locationInfo[cellAndSectorID].latitude = cellInfoList[7].toDouble();
		}
	}
}

void DropCallAnalyse::initShortID2LongIDMap()
{
	auto itConnectInfo = m_cellConnectInfo.begin();
	while (itConnectInfo != m_cellConnectInfo.end())
	{
		QStringList longID = itConnectInfo.key().split('_');
		QString shortID = longID[2] + "_" + longID[3] + "_" + longID[4];
		m_shortID2LongID[shortID] = itConnectInfo.key();
		++itConnectInfo;
	}
}

void DropCallAnalyse::analyseMMLChange()
{
	QMap<QString, QStringList>yesterMMLParams;

	QString yesterdayFileName = QDateTime::currentDateTime().addDays(-2).toString("yyyyMMdd") + "MMLrecord.csv";
	yesterdayFileName = m_dataDir + yesterdayFileName;

	QFile yesterdayFile(yesterdayFileName);
	if (yesterdayFile.open(QIODevice::ReadOnly))
	{
		QTextStream yesReadStream(&yesterdayFile);
		yesReadStream.readLine();

		while (!yesReadStream.atEnd())
		{
			QString line = yesReadStream.readLine();
			int starPos = 0;
			int endPos = line.indexOf(',', starPos);
			QString carrierStr = line.mid(starPos, endPos - starPos).trimmed();

			starPos = endPos + 1;
			QString parameters = line.mid(starPos);

			yesterMMLParams[carrierStr] = parameters.split(',');
		}
	}

	QVector<QString> mmlChangeCells;

	{
		QString todayFileName = QDateTime::currentDateTime().addDays(-1).toString("yyyyMMdd") + "MMLrecord.csv";
		todayFileName = m_dataDir + todayFileName;
		QString tmpFileName = QDateTime::currentDateTime().addDays(-1).toString("yyyyMMdd") + "MMLrecord1.csv";
		tmpFileName = m_dataDir + tmpFileName;

		QFile todayFile(todayFileName);
		todayFile.open(QIODevice::ReadOnly);
		QFile tmpFile(tmpFileName);
		tmpFile.open(QIODevice::WriteOnly);

		QTextStream todRWStream(&todayFile);
		QTextStream tmpWriteStream(&tmpFile);
		tmpWriteStream << todRWStream.readLine() << "\n";

		while (!todRWStream.atEnd())
		{
			QString line = todRWStream.readLine();
			tmpWriteStream << line;

			int starPos = 0;
			int endPos = line.indexOf(',', starPos);
			QString carrierStr = line.mid(starPos, endPos - starPos).trimmed();
			QString PnDist = m_dropReasonInfoByH.contains(carrierStr) ? m_dropReasonInfoByH[carrierStr][25] : "";

			if (!yesterMMLParams.contains(carrierStr))
			{
				tmpWriteStream << "," << PnDist << "," << "no" << "\n";
				continue;
			}

			starPos = endPos + 1;
			QString parameters = line.mid(starPos);
			QStringList parameterList = parameters.split(',');
			const QStringList& yesterMMLParamsRef = yesterMMLParams[carrierStr];

			bool bSame = true;
			if (yesterMMLParamsRef.size() == parameterList.size() - 2)
			{
				for (int i = 1; i < parameterList.size(); ++i)
				{
					if (yesterMMLParamsRef[i] != parameterList[i])
					{
						bSame = false;
						break;
					}
				}
			}
			else
				bSame = false;
			
			if (bSame)
				bSame = (PnDist == yesterMMLParamsRef[13]);

			if (bSame)
				tmpWriteStream << "," << PnDist << "," << "no";
			else
			{
				tmpWriteStream << "," << PnDist << "," << "yes";
				mmlChangeCells.push_back(carrierStr);

				QString dropReason("参数变化");
				DropReasonInfo& dropReasonInfoRef = m_dropReasonInfo[carrierStr];
				QString dropReasonDetail;
				for (int i = 1; i < parameterList.size() - 2; ++i)
				{
					dropReasonDetail += (parameterList[i] + ",");
				}
				dropReasonDetail += PnDist;
				dropReasonInfoRef.dropReasonMap[dropReason].push_back(dropReasonDetail);
			}
			tmpWriteStream << "\n";
		}

		todayFile.close();
		todayFile.remove();
		tmpFile.close();
		tmpFile.rename(todayFileName);

	}

	///topN 是否变化的比较
	QFile topNFile(m_dataDir + QDateTime::currentDateTime().addDays(-1).toString("yyyyMMdd") + "topNFile.csv");
	topNFile.open(QIODevice::WriteOnly);
	QTextStream writeStream(&topNFile);

	writeStream << QObject::tr("载波,邻区的MML是否变化") << "\n";

	for (int i = 0; i < m_topNCells.size(); ++i)
	{
		writeStream << m_topNCells[i] << ",";

		const QStringList& neighborCellsRef = m_neighborCells[m_topNCells[i]];
		QVector<int> changeCells;
		QVector<QString> changeCellIDs;

		for (int j = 0; j < neighborCellsRef.size(); ++j)
		{
			if (mmlChangeCells.contains(neighborCellsRef[j]))
			{
				changeCells.push_back(j);
				changeCellIDs.push_back(neighborCellsRef[j]);
			}
		}

		if (changeCells.isEmpty())
			writeStream << QObject::tr("否");
		else
		{
			writeStream << QObject::tr("是");
			for (int j = 0; j < changeCells.size(); ++j)
				writeStream << ";" + changeCellIDs[j] + "-" + QString::number(changeCells[j]) + "-优先级";
		}
		writeStream << "\n";
	}

	topNFile.close();
}

void DropCallAnalyse::exportHtml()
{
	QDir dataDir(m_dataDir);
	dataDir.mkdir(QDateTime::currentDateTime().addDays(-1).toString("yyyyMMdd"));
	QString htmlDir = m_dataDir + "/" + QDateTime::currentDateTime().addDays(-1).toString("yyyyMMdd");

	QFile mainFile(htmlDir + "/main.html");
	mainFile.open(QIODevice::WriteOnly);
	QTextStream writeStream(&mainFile);
	QTextCodec* codec= QTextCodec::codecForName("UTF-8");
	writeStream.setCodec(codec);
	writeHead(writeStream, QObject::tr("指标问题自动化分析系统"));
	writeTBody(writeStream, m_topNCells, m_cellConnectInfo);
	QVector<int> emptyVec;
	writeCanvas(writeStream, 0, emptyVec, "连接次数");
	writeStream << "</tr>\n<tr>\n";
	writeCanvas(writeStream, 1, emptyVec, "掉话次数");
	writeDropReason(writeStream, DropReasonInfo(), AlarmInfo(), 0);
	writeCanvasDropReason(writeStream, emptyVec);
	writeEnd(writeStream);
	mainFile.close();
}

void DropCallAnalyse::exportTopNHtml()
{
	QString htmlDir = m_dataDir + "/" + QDateTime::currentDateTime().addDays(-1).toString("yyyyMMdd");

	for (int idx = 1; idx <= m_topNCells.size(); ++idx)
	{
		QFile topNFile(htmlDir + "/TOP" + QString::number(idx) + ".html");
		topNFile.open(QIODevice::WriteOnly);
		QTextStream writeStream(&topNFile);
		QTextCodec* codec= QTextCodec::codecForName("UTF-8");
		writeStream.setCodec(codec);
		writeHead(writeStream, "TOP" + QString::number(idx));
		writeTBody(writeStream, m_topNCells, m_cellConnectInfo);
		QVector<int> countByH(24);
		QString cellID = m_topNCells[idx-1];
		for (int i = 0; i < 24; ++i)
			countByH[i] = m_cellConnectInfo[cellID].connectByH[i];
		writeCanvas(writeStream, 0, countByH, "连接次数");
		writeStream << "</tr>\n<tr>\n";
		for (int i = 0; i < 24; ++i)
			countByH[i] = m_cellConnectInfo[cellID].dropCntByH[i];
		writeCanvas(writeStream, 1, countByH, "掉话次数");
		writeDropReason(writeStream, m_dropReasonInfo[cellID], m_alarmInfo[cellID], idx);
		writeCanvasDropReason(writeStream, QVector<int>());
		writeEnd(writeStream);
		topNFile.close();

		exportDropReasonHtml(idx);
	}

}

void DropCallAnalyse::exportDropReasonHtml(int topN)
{
	QString htmlDir = m_dataDir + "/" + QDateTime::currentDateTime().addDays(-1).toString("yyyyMMdd");
	static QString idxs[19] = {
		"CE不足", "RSSI问题", "传输问题", "锁星问题", "驻波比问题", "小区退服",
		"其它", "NULL", "邻区漏配", "优先级不合理", "参数变化", "新增载波",
		"载波关停", "载波长期关停", "载波恢复", "弱覆盖", "越区覆盖", "用户行为", "掉话类型"};

	QString cellID = m_topNCells[topN-1];
	QVector<int> connectByH(24);
	QVector<int> dropCountByH(24);
	for (int i = 0; i < 24; ++i)
	{
		connectByH[i] = m_cellConnectInfo[cellID].connectByH[i];
		dropCountByH[i] = m_cellConnectInfo[cellID].dropCntByH[i];
	}

	const AlarmInfo& alarmInfoRef = m_alarmInfo[cellID];
	const QMap<QString, TypeCountInfo>& typeCountMapRef = alarmInfoRef.typeCountMap;
	const DropReasonInfo& dropReasonInfoRef = m_dropReasonInfo[cellID];
	const QMap<QString, QVector<QString> >& dropReasonMapRef = dropReasonInfoRef.dropReasonMap;

	auto itAlarm = typeCountMapRef.begin();
	while (itAlarm != typeCountMapRef.end())
	{
		int idx = -1;
		for (int i = 0; i < 19; ++i)
		{
			if (itAlarm.key() == idxs[i])
			{
				idx = i + 1;
				break;
			}
		}

		if (idx == -1)
		{
			++itAlarm;
			continue;
		}

		QFile topNFile(htmlDir + "/TOP" + QString::number(topN) + "_" + QString::number(idx) + ".html");
		topNFile.open(QIODevice::WriteOnly);
		QTextStream writeStream(&topNFile);
		QTextCodec* codec= QTextCodec::codecForName("UTF-8");
		writeStream.setCodec(codec);
		writeHead(writeStream, "TOP" + QString::number(topN) + "_" + itAlarm.key());
		writeTBody(writeStream, m_topNCells, m_cellConnectInfo);
		writeCanvas(writeStream, 0, connectByH, "连接次数");
		writeStream << "</tr>\n<tr>\n";
		writeCanvas(writeStream, 1, dropCountByH, "掉话次数");
		writeDropReason(writeStream, dropReasonInfoRef, alarmInfoRef, topN);
		QVector<int> typeCount(24);
		for (int i = 0; i < 24; ++i)
			typeCount[i] = itAlarm.value().typeCount[i];
		writeCanvasDropReason(writeStream, typeCount, QVector<QString>(), itAlarm.key());
		writeEnd(writeStream);
		topNFile.close();
		++ itAlarm;
	}

	auto itDropReason = dropReasonMapRef.begin();
	while (itDropReason != dropReasonMapRef.end())
	{
		int idx = -1;
		for (int i = 0; i < 19; ++i)
		{
			if (itDropReason.key() == idxs[i])
			{
				idx = i + 1;
				break;
			}
		}

		if (idx == -1)
		{
			++itDropReason;
			continue;
		}

		QFile topNFile(htmlDir + "/TOP" + QString::number(topN) + "_" + QString::number(idx) + ".html");
		topNFile.open(QIODevice::WriteOnly);
		QTextStream writeStream(&topNFile);
		QTextCodec* codec= QTextCodec::codecForName("UTF-8");
		writeStream.setCodec(codec);
		writeHead(writeStream, "TOP" + QString::number(topN) + "_" + itDropReason.key());
		writeTBody(writeStream, m_topNCells, m_cellConnectInfo);
		writeCanvas(writeStream, 0, connectByH, "连接次数");
		writeStream << "</tr>\n<tr>\n";
		writeCanvas(writeStream, 1, dropCountByH, "掉话次数");
		writeDropReason(writeStream, dropReasonInfoRef, alarmInfoRef, topN);
		if (idx > 8 && idx < 16)
			writeCanvasDropReason(writeStream, QVector<int>(), itDropReason.value());
		else
			writeCanvasDropReason(writeStream, QVector<int>());
		writeEnd(writeStream);
		topNFile.close();
		++itDropReason;
	}
}