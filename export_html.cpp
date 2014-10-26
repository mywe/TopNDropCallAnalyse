#include "export_html.h"

void writeHead(QTextStream& writeStream, const QString& title)
{
	writeStream << "<html>\n<head> \n"
				<< "<title>" << title << "</title> \n"
			    << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"/> \n"
				<< CSS_DEFINE
				<< QObject::tr("性能自动化分析系统") << "\n"
				<< "</p>\n"
				//<< "<h1>" << QObject::tr("性能自动化分析系统") << "</h1>\n"
				<< "<h2>" << QObject::tr("TOP10载波") << "</h2>\n"
				<< "<table>\n"
				<< "<tr>\n"
				<< "<td rowspan=\"2\">\n"
				<< "<div style='fixed:left'>\n"
				<< "<table class=\"table1\">\n"
				<< "<thead>\n<tr>\n<th></th>\n"
				<< "<th scope=\"col\" abbr=\"Carrier\">" << QObject::tr("载波") << "</th>\n"
				<< "<th scope=\"col\" abbr=\"Name\">" << QObject::tr("站名") << "</th>\n"
				<< "<th scope=\"col\" abbr=\"DropCount\">" << QObject::tr("掉话次数") << "</th>\n"
				<< "<th scope=\"col\" abbr=\"ConnCount\">" << QObject::tr("呼叫次数") << "</th>\n"
				<< "<th scope=\"col\" abbr=\"Rate\">" << QObject::tr("掉话率") << "</th>\n"
				<< "<th scope=\"col\" abbr=\"ECIOAvg\">" << QObject::tr("平均ECIO") << "</th>\n"
				<< "<th scope=\"col\" abbr=\"DistanceAvg\">" << QObject::tr("平均掉话距离") << "</th>\n"
				<< "<th scope=\"col\" abbr=\"MainRSSIAvg\">" << QObject::tr("RSSI主集均值") << "</th>\n"
				<< "<th scope=\"col\" abbr=\"MinorRSSIAvg\">" << QObject::tr("RSSI分集均值") << "</th>\n"
				<< "</tr>\n</thead>\n";
}

void writeTBody(QTextStream& writeStream,
				const QVector<QString>& topNCells,
				const QMap<QString, CellConnectInfo>& cellConnectInfo)
{
	writeStream << "<tbody>\n";
	for (int i = 0; i < 10; ++i)
	{
		if (i < topNCells.size())
		{
			const CellConnectInfo& cellConnectInfoRef = cellConnectInfo[topNCells[i]];
			QString topNum = "TOP" + QString::number(i+1);
			qreal rateOfDrop = cellConnectInfoRef.nConnect!=0 ? cellConnectInfoRef.nDropCall / (qreal)cellConnectInfoRef.nConnect : 0;
			writeStream << "<tr>\n"
				<< "<th scope=\"row\">" << topNum << "</th>\n"
				<< "<td>" << "<a href=" << topNum + ".html>" << topNCells[i] << "</a></td>\n"
				<< "<td>" << "<a href=" << topNum + ".html>" << cellConnectInfoRef.cellName << "</a></td>\n"
				<< "<td>" << "<a href=" << topNum + ".html>" << cellConnectInfoRef.nDropCall << "</a></td>\n"
				<< "<td>" << "<a href=" << topNum + ".html>" << cellConnectInfoRef.nConnect << "</a></td>\n"
				<< "<td>" << "<a href=" << topNum + ".html>" << QString::number(rateOfDrop, 'g', 2) << "</a></td>\n"
				<< "<td>" << "<a href=" << topNum + ".html>" << cellConnectInfoRef.fAvgEcio << "</a></td>\n"
				<< "<td>" << "<a href=" << topNum + ".html>" << cellConnectInfoRef.fAvgDropDist << "</a></td>\n"
				<< "<td>" << "<a href=" << topNum + ".html>" << cellConnectInfoRef.fAvgOfMainRSSI << "</a></td>\n"
				<< "<td>" << "<a href=" << topNum + ".html>" << cellConnectInfoRef.fAvgOfSubRSSI << "</a></td>\n"
				<< "</tr>\n";
		}
		else
		{
			writeStream << "<tr>\n"
				<< "<th scope=\"row\">TOP" << i+1 << "</th>\n"
				<< "<td>" << "</td>\n"
				<< "<td>" << "</td>\n"
				<< "<td>" << "</td>\n"
				<< "<td>" << "</td>\n"
				<< "<td>" << "</td>\n"
				<< "<td>" << "</td>\n"
				<< "<td>" << "</td>\n"
				<< "<td>" << "</td>\n"
				<< "<td>" << "</td>\n"
				<< "</tr>\n";
		}
	}
	writeStream << "</tbody>\n</table>\n</div>\n";
}

void writeEnd(QTextStream& writeStream)
{
	writeStream 
		<< "<div id=\"footer\" style=\"text-align:center\">"
		<< "<p>" << QObject::tr("佛山无线网优中心") << "</p>"
		<< "<p>" <<QObject::tr("修改建议请联系彭雯婷:18144797979") << "</p>"
		<< "</div>\n</body>\n</html>";
}

QString genTendencyChart(const QVector<int>& countByH)
{
	int maxVal = 0;
	for (int i = 0; i < countByH.size(); ++i)
	{
		maxVal = qMax(countByH[i], maxVal);
	}

	QVector<int> xByH(24, 0);
	QVector<int> yByH(24, 0);

	if (maxVal)
	{
		for (int i = 0; i < 24; ++i)
		{
			xByH[i] = 20 + 18 * (i + 1);
			yByH[i] = 270 - (countByH[i] / (double)maxVal) * 250;
		}
	}
	
	QString resultStr("function drawChart(){\n");
	resultStr += "ctx.save();\n";

	if (maxVal)
	{
		resultStr += "ctx.strokeStyle='rgba(255,0,0,1)';\n";
		resultStr += "ctx.beginPath();\n";
		resultStr += "ctx.moveTo(" + QString::number(xByH[0]) + ","
			+ QString::number(yByH[0]) + ");\n";
		for (int i = 1; i < 24; ++i)
		{
			resultStr += "ctx.lineTo(" + QString::number(xByH[i]) + ","
				+ QString::number(yByH[i]) + ");\n";
		}
		resultStr += "ctx.stroke();\n";
	}

	resultStr += "ctx.restore();\n}";
	return resultStr;
}

void writeCanvas(QTextStream& writeStream, int num,
					const QVector<int>& countByH, const QString& canvasName)
{
	writeStream << "<td>\n" << canvasName << "<div style='float:left'>\n"
		<< "<canvas width='480' height='300' id='canvas" << num <<"' style='border:1px solid'>\n"
		<< "</canvas>\n<script>\n"
		<< "var canvas=document.getElementById('canvas" << num << "');\n"
		<< "var ctx=canvas.getContext('2d');\n"
		<< AXIS_DEFINE << "\n";
	
	if (countByH.size())
		writeStream << genTendencyChart(countByH);
	
	writeStream << "function draw(){\n"
		<< "ctx.clearRect(0,0,canvas.width,canvas.height);\n"
		<< "ctx.save();\n"
		<< "drawAxis();\n";
	
	if (countByH.size())
		writeStream << "drawChart();\n";

	writeStream << "ctx.restore();\n"
		<< "}\n"
		<< "ctx.fillStyle='rgba(100,140,230,0.5)';\n"
		<< "ctx.strokeStyle='rgba(33,33,33,1)';\n"
		<< "draw();\n</script>\n</div>\n</td>\n";
}

void writeCanvasDropReason(QTextStream& writeStream, const QVector<int>& countByH,
							const QVector<QString>& dropReason, const QString& canvasName)
{
	writeStream << "<td rowspan=\"2\">\n" << canvasName << "<div style='float:left'>\n"
		<< "<canvas width='480' height='300' id='canvas2" <<"' style='border:1px solid'>\n"
		<< "</canvas>\n<script>\n"
		<< "var canvas=document.getElementById('canvas2" << "');\n"
		<< "var ctx=canvas.getContext('2d');\n"
		<< AXIS_DEFINE << "\n";

	if (countByH.size())
		writeStream << genTendencyChart(countByH);

	writeStream << "function draw(){\n"
		<< "ctx.clearRect(0,0,canvas.width,canvas.height);\n"
		<< "ctx.save();\n"
		<< "drawAxis();\n";

	if (countByH.size())
		writeStream << "drawChart();\n";

	writeStream << "ctx.restore();\n"
		<< "}\n"
		<< "ctx.fillStyle='rgba(100,140,230,0.5)';\n"
		<< "ctx.strokeStyle='rgba(33,33,33,1)';\n"
		<< "draw();\n</script>\n</div>\n</td>\n</tr>\n";
	
	if (dropReason.size())
	{
		writeStream << "<tr>\n<td>\n";
		for (int i = 0; i < dropReason.size() - 1; ++i)
			writeStream << dropReason[i] << ",";
		writeStream << dropReason[dropReason.size() - 1] << "\n";
		writeStream << "</td>\n</tr>\n";
	}

	writeStream << "</table>\n</div>\n";
}

void writeDropReason(QTextStream& writeStream,
						const DropReasonInfo& dropReasonRef,
						const AlarmInfo& alarmInfoRef,
						int topN)
{
	// red:f12339
	// green:9dd929
	writeStream << "</tr>\n</table>\n"
		<< "<h2>" << QObject::tr("掉话原因") << "</h2>\n"
		<< "<div>\n<table>\n<tr>\n<td>\n<div style='fixed:left'>\n<table>\n<tr>\n"
		<< "<td bgcolor=\"#d1d1d1\" style=\"border:5px solid #d1d1d1\">" << QObject::tr("设备类") << "</td>\n";

	if (alarmInfoRef.typeCountMap.contains("CE不足"))
		writeStream << "<td bgcolor=\"f12339\" style=\"text-align:center\">"
					<< "<a href=TOP" << topN << "_1.html>" << QObject::tr("CE不足") << "</a></td>\n";
	else
		writeStream << "<td bgcolor=\"9dd929\" style=\"text-align:center\">" << QObject::tr("CE不足") << "</td>\n";
	
	if (alarmInfoRef.typeCountMap.contains("RSSI问题"))
		writeStream << "<td bgcolor=\"f12339\" style=\"text-align:center\">"
					<< "<a href=TOP" << topN << "_2.html>" << QObject::tr("RSSI问题") << "</a></td>\n";
	else
		writeStream << "<td bgcolor=\"9dd929\" style=\"text-align:center\">" << QObject::tr("RSSI问题") << "</td>\n";

	if (alarmInfoRef.typeCountMap.contains("传输问题"))
		writeStream << "<td bgcolor=\"f12339\" style=\"text-align:center\">"
					<< "<a href=TOP" << topN << "_3.html>" << QObject::tr("传输问题") << "</a></td>\n";
	else
		writeStream << "<td bgcolor=\"9dd929\" style=\"text-align:center\">" << QObject::tr("传输问题") << "</td>\n";

	if (alarmInfoRef.typeCountMap.contains("锁星问题"))
		writeStream << "<td bgcolor=\"f12339\" style=\"text-align:center\">"
					<< "<a href=TOP" << topN << "_4.html>" << QObject::tr("锁星问题") << "</a></td>\n";
	else
		writeStream << "<td bgcolor=\"9dd929\" style=\"text-align:center\">" << QObject::tr("锁星问题") << "</td>\n";

	if (alarmInfoRef.typeCountMap.contains("驻波比问题"))
		writeStream << "<td bgcolor=\"f12339\" style=\"text-align:center\">"
					<< "<a href=TOP" << topN << "_5.html>" << QObject::tr("驻波比问题") << "</a></td>\n";
	else
		writeStream << "<td bgcolor=\"9dd929\" style=\"text-align:center\">" << QObject::tr("驻波比问题") << "</td>\n";

	if (alarmInfoRef.typeCountMap.contains("小区退服"))
		writeStream << "<td bgcolor=\"f12339\" style=\"text-align:center\">"
					<< "<a href=TOP" << topN << "_6.html>" << QObject::tr("小区退服") << "</a></td>\n";
	else
		writeStream << "<td bgcolor=\"9dd929\" style=\"text-align:center\">" << QObject::tr("小区退服") << "</td>\n";

	if (alarmInfoRef.typeCountMap.contains("其它"))
		writeStream << "<td bgcolor=\"f12339\" style=\"text-align:center\">"
					<< "<a href=TOP" << topN << "_7.html>" << QObject::tr("其他问题") << "</a></td>\n";
	else
		writeStream << "<td bgcolor=\"9dd929\" style=\"text-align:center\">" << QObject::tr("其他问题") << "</td>\n";

	if (alarmInfoRef.typeCountMap.contains("NULL"))
		writeStream << "<td bgcolor=\"f12339\" style=\"text-align:center\">"
					<< "<a href=TOP" << topN << "_8.html>" << QObject::tr("不影响业务") << "</a></td>\n";
	else
		writeStream << "<td bgcolor=\"9dd929\" style=\"text-align:center\">" << QObject::tr("不影响业务") << "</td>\n";

	writeStream << "</tr>\n<tr>\n"
		<< "<td bgcolor=\"#d1d1d1\" style=\"border:5px solid #d1d1d1\">" << QObject::tr("参数类") << "</td>\n";

	if (dropReasonRef.dropReasonMap.contains("邻区漏配"))
		writeStream << "<td bgcolor=\"f12339\"; colspan=\"3\" style=\"text-align:center\">"
					<< "<a href=TOP" << topN << "_9.html>" << QObject::tr("邻区漏配") << "</a></td>\n";
	else
		writeStream << "<td bgcolor=\"9dd929\"; colspan=\"3\" style=\"text-align:center\">" << QObject::tr("邻区漏配") << "</td>\n";

	if (dropReasonRef.dropReasonMap.contains("优先级不合理"))
		writeStream << "<td bgcolor=\"f12339\"; colspan=\"2\" style=\"text-align:center\">"
					<< "<a href=TOP" << topN << "_10.html>" << QObject::tr("优先级不合理") << "</a></td>\n";
	else
		writeStream << "<td bgcolor=\"9dd929\"; colspan=\"2\" style=\"text-align:center\">" << QObject::tr("优先级不合理") << "</td>\n";

	if (dropReasonRef.dropReasonMap.contains("参数变化"))
		writeStream << "<td bgcolor=\"f12339\"; colspan=\"3\" style=\"text-align:center\">"
					<< "<a href=TOP" << topN << "_11.html>" << QObject::tr("参数变化") << "</a></td>\n";
	else
		writeStream << "<td bgcolor=\"9dd929\"; colspan=\"3\" style=\"text-align:center\">" << QObject::tr("参数变化") << "</td>\n";

	writeStream << "</tr>\n<tr>\n"
		<< "<td bgcolor=\"#d1d1d1\" style=\"border:5px solid #d1d1d1\">" << QObject::tr("周边基站影响") << "</td>\n";

	if (dropReasonRef.dropReasonMap.contains("新增载波"))
		writeStream << "<td bgcolor=\"f12339\"; colspan=\"2\" style=\"text-align:center\">"
					<< "<a href=TOP" << topN << "_12.html>" << QObject::tr("新增载波") << "</a></td>\n";
	else
		writeStream << "<td bgcolor=\"9dd929\"; colspan=\"2\" style=\"text-align:center\">" << QObject::tr("新增载波") << "</td>\n";

	if (dropReasonRef.dropReasonMap.contains("载波关停"))
		writeStream << "<td bgcolor=\"f12339\"; colspan=\"2\" style=\"text-align:center\">"
					<< "<a href=TOP" << topN << "_13.html>" << QObject::tr("载波关停") << "</a></td>\n";
	else
		writeStream << "<td bgcolor=\"9dd929\"; colspan=\"2\" style=\"text-align:center\">" << QObject::tr("载波关停") << "</td>\n";

	if (dropReasonRef.dropReasonMap.contains("载波长期关停"))
		writeStream << "<td bgcolor=\"f12339\"; colspan=\"3\" style=\"text-align:center\">"
					<< "<a href=TOP" << topN << "_14.html>" << QObject::tr("载波长期关停") << "</a></td>\n";
	else
		writeStream << "<td bgcolor=\"9dd929\"; colspan=\"3\" style=\"text-align:center\">" << QObject::tr("载波长期关停") << "</td>\n";

	if (dropReasonRef.dropReasonMap.contains("载波恢复"))
		writeStream << "<td bgcolor=\"f12339\"; colspan=\"2\" style=\"text-align:center\">"
					<< "<a href=TOP" << topN << "_15.html>" << QObject::tr("载波恢复") << "</a></td>\n";
	else
		writeStream << "<td bgcolor=\"9dd929\"; colspan=\"2\" style=\"text-align:center\">" << QObject::tr("载波恢复") << "</td>\n";

	writeStream << "</tr>\n<tr>\n"
		<< "<td bgcolor=\"#d1d1d1\" style=\"border:5px solid #d1d1d1\">" << QObject::tr("覆盖类") << "</td>\n";

	if (dropReasonRef.dropReasonMap.contains("弱覆盖"))
		writeStream << "<td bgcolor=\"f12339\"; colspan=\"5\" style=\"text-align:center\">"
					<< "<a href=TOP" << topN << "_16.html>" << QObject::tr("弱覆盖") << "</a></td>\n";
	else
		writeStream << "<td bgcolor=\"9dd929\"; colspan=\"5\" style=\"text-align:center\">" << QObject::tr("弱覆盖") << "</td>\n";

	if (dropReasonRef.dropReasonMap.contains("越区覆盖"))
		writeStream << "<td bgcolor=\"f12339\"; colspan=\"4\" style=\"text-align:center\">"
					<< "<a href=TOP" << topN << "_17.html>" << QObject::tr("越区覆盖") << "</a></td>\n";
	else
		writeStream << "<td bgcolor=\"9dd929\"; colspan=\"4\" style=\"text-align:center\">" << QObject::tr("越区覆盖") << "</td>\n";

	writeStream << "</tr>\n<tr>\n"
		<< "<td bgcolor=\"#d1d1d1\" style=\"border:5px solid #d1d1d1\">" << QObject::tr("其他") << "</td>\n";
	
	if (dropReasonRef.dropReasonMap.contains("用户行为"))
		writeStream << "<td bgcolor=\"f12339\"; colspan=\"4\" style=\"text-align:center\">"
					<< "<a href=TOP" << topN << "_18.html>" << QObject::tr("用户行为导致高掉话") << "</a></td>\n";
	else
		writeStream << "<td bgcolor=\"9dd929\"; colspan=\"4\" style=\"text-align:center\">" << QObject::tr("用户行为导致高掉话") << "</td>\n";

	if (dropReasonRef.dropReasonMap.contains("掉话类型"))
		writeStream << "<td bgcolor=\"f12339\"; colspan=\"5\" style=\"text-align:center\">"
					<< "<a href=TOP" << topN << "_19.html>" << QObject::tr("掉话类型") << "</a></td>\n";
	else
		writeStream << "<td bgcolor=\"9dd929\"; colspan=\"5\" style=\"text-align:center\">" << QObject::tr("掉话类型") << "</td>\n";

	writeStream<< "</tr>\n</table>\n</div>\n</td>\n";
}