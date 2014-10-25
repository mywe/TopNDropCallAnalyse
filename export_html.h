#ifndef __EXPORT_HTML_H__
#define __EXPORT_HTML_H__

#include <QTextStream>
#include <QVector>
#include <QMap>
#include <QString>

const int alarmHoursCount = 24;
struct TypeCountInfo
{
public:
	TypeCountInfo()
	{
		for(int i = 0; i < alarmHoursCount; ++i)
			typeCount[i] = 0;
	}
	QString name;
	int typeCount[alarmHoursCount];
};

struct AlarmInfo
{
public:
	QString name;
	QString alarmName;
	QMap<QString, TypeCountInfo> typeCountMap;
};

struct DropReasonInfo
{
	QMap<QString, QVector<QString> > dropReasonMap;
};

struct CellConnectInfo
{
	CellConnectInfo()
		: nDropCall(0)
		, nConnect(0)
		, fAvgDropDist(0)
		, fAvgEcio(0)
		, fAvgOfMainRSSI(0)
		, fAvgOfSubRSSI(0)
	{
		for (int i = 0; i < 24; ++i)
		{
			connectByH[i] = 0;
			dropCntByH[i] = 0;
		}

		for (int i = 0; i < 22; ++i)
		{
			dropCntByD[i] = 0;
			ecioByD[i] = 0;
		}
	}

	int nDropCall;
	int nConnect;
	qreal fAvgDropDist;
	qreal fAvgEcio;
	qreal fAvgOfMainRSSI;
	qreal fAvgOfSubRSSI;
	int	connectByH[24];
	int dropCntByH[24];
	int dropCntByD[22];
	qreal ecioByD[22];
	QString cellName;
};

void writeHead(QTextStream& writeStream, const QString& title = QString());
void writeTBody(QTextStream& writeStream,
				const QVector<QString>& topNCells,
				const QMap<QString, CellConnectInfo>& cellConnectInfo);
void writeEnd(QTextStream& writeStream);
void writeCanvas(QTextStream& writeStream, int num,
					const QVector<int>& countByH, const QString& canvasName = QString());
void writeCanvasDropReason(QTextStream& writeStream,
							const QVector<int>& countByH,
							const QVector<QString>& dropReason = QVector<QString>(),
							const QString& canvasName = QString());
void writeDropReason(QTextStream& writeStream,
						const DropReasonInfo& dropReasonRef,
						const AlarmInfo& alarmInfoRef,
						int topN);

#define CSS_DEFINE \
	"<style> \n\
	table.table1 \n\
	{ \n\
		font-family: \"Trebuchet MS\", sans-serif; \n\
		font-size: 13px; \n\
		font-weight: bold; \n\
		line-height: 1.2em; \n\
		font-style: normal; \n\
		border-collapse:separate; \n\
	} \n\
	.table1 thead th \n\
	{ \n\
		padding:15px;\n\
		color:#fff;\n\
		text-shadow:1px 1px 1px #568F23;\n\
		border:1px solid #93CE37; \n\
		border-bottom:3px solid #9ED929;\n\
		background-color:#9DD929;\n\
		background:-webkit-gradient\n\
				   ( \n\
				   linear,\n\
				   left bottom,\n\
				   left top,\n\
				   color-stop(0.02, rgb(123,192,67)),\n\
				   color-stop(0.51, rgb(139,198,66)),\n\
				   color-stop(0.87, rgb(158,217,41))\n\
				   );\n\
		background: -moz-linear-gradient \n\
					(\n\
					center bottom,\n\
					rgb(123,192,67) 2%,\n\
					rgb(139,198,66) 51%,\n\
					rgb(158,217,41) 87%\n\
					);\n\
		-webkit-border-top-left-radius:5px;\n\
		-webkit-border-top-right-radius:5px;\n\
		-moz-border-radius:5px 5px 0px 0px;\n\
		border-top-left-radius:5px;\n\
		border-top-right-radius:5px;\n\
	}\n\
	.table1 thead th:empty\n\
	{\n\
	background:transparent;\n\
	border:none;\n\
	}\n\
	.table1 tbody th\n\
	{\n\
		color:#fff;\n\
		text-shadow:1px 1px 1px #568F23;\n\
		background-color:#9DD929;\n\
		border:1px solid #93CE37;\n\
		border-right:3px solid #9ED929;\n\
		padding:0px 10px;\n\
		background:-webkit-gradient\n\
				   (\n\
				   linear,\n\
				   left bottom,\n\
				   right top,\n\
				   color-stop(0.02, rgb(158,217,41)),\n\
				   color-stop(0.51, rgb(139,198,66)),\n\
				   color-stop(0.87, rgb(123,192,67))\n\
				   );\n\
		background: -moz-linear-gradient\n\
					(\n\
					left bottom,\n\
					rgb(158,217,41) 2%,\n\
					rgb(139,198,66) 51%,\n\
					rgb(123,192,67) 87%\n\
					);\n\
		-moz-border-radius:5px 0px 0px 5px;\n\
		-webkit-border-top-left-radius:5px;\n\
		-webkit-border-bottom-left-radius:5px;\n\
		border-top-left-radius:5px;\n\
		border-bottom-left-radius:5px;\n\
	}\n\
	.table1 tbody td\n\
	{\n\
	padding:10px;\n\
	text-align:center;\n\
	background-color:#DEF3CA;\n\
	border: 2px solid #E7EFE0;\n\
	-moz-border-radius:2px;\n\
	-webkit-border-radius:2px;\n\
	border-radius:2px;\n\
	color:#666;\n\
	text-shadow:1px 1px 1px #fff;\n\
	}\n\
	</style>\n\
		</head>\n\
		<style>\n\
		*{\n\
	margin:0;\n\
	padding:0;\n\
	}\n\
	body\n\
	{\n\
		height: 100%\n\
		font-family: Georgia, serif;\n\
		font-size: 20px;\n\
		font-style: normal;\n\
		font-weight: normal;\n\
		letter-spacing: normal;\n\
		background: #ffffff;\n\
		border-left:20px solid #1D81B6;\n\
		-moz-box-shadow:0px 0px 16px #aaa;\n\
	}\n\
	#content\n\
	{\n\
		background-color:#fff;\n\
		width:auto;\n\
		padding:40px;\n\
		margin:0 auto;\n\
	}\n\
	.head\n\
	{\n\
		font-family:\"Trebuchet MS\",sans-serif;\n\
		color:#1D81B6;\n\
		font-weight:normal;\n\
		font-size:40px;\n\
		font-style:normal;\n\
		letter-spacing:3px;\n\
		border-bottom:3px solid #f0f0f0;\n\
		padding-bottom:10px;\n\
		margin-bottom:10px;\n\
	}\n\
	.head a\n\
	{\n\
		color:#1D81B6;\n\
		text-decoration:none;\n\
		float:right;\n\
		text-shadow:1px 1px 1px #888;\n\
	}\n\
	.head a:hover\n\
	{\n\
	color:#f0f0f0;\n\
	}\n\
	#content h1\n\
	{\n\
		font-family:\"Trebuchet MS\",sans-serif;\n\
		color:#1D81B6;\n\
		font-weight:normal;\n\
		font-style:normal;\n\
		font-size:40px;\n\
		text-shadow:1px 1px 1px #aaa;\n\
	}\n\
	#content h2\n\
	{\n\
		font-family:\"Trebuchet MS\",sans-serif;\n\
		font-size:15px;\n\
		font-style:normal;\n\
		background-color:#f0f0f0;\n\
		margin:40px 0px 30px -40px;\n\
		padding:0px 40px;\n\
		clear:both;\n\
		width:10%;\n\
		color:#aaa;\n\
		text-shadow:1px 1px 1px #fff;\n\
	}\n\
	#footer\n\
	{\n\
		font-family:Helvetica,Arial,Verdana;\n\
		text-transform:uppercase;\n\
		font-size:5px;\n\
		font-style:normal;\n\
		background-color:#f0f0f0;\n\
		padding-top:10px;\n\
		padding-bottom:10px;\n\
	}\n\
	</style>\n\
		<body id=\"body\">\n\
		<div id=\"content\" style='text-align:center'>\n\
		<a class=\"back\" href=\"\"></a>\n\
		<span class=\"scroll\"></span>\n\
		<p class=\"head\">\n"

#define AXIS_DEFINE \
	"function drawAxis(){\n\
		ctx.beginPath();\n\
		ctx.moveTo(20,280);\n\
		ctx.lineTo(470,280);\n\
		ctx.lineTo(462,283);\n\
		ctx.lineTo(462,277);\n\
		ctx.lineTo(470,280);\n\
		ctx.stroke();\n\
		ctx.moveTo(20,280);\n\
		ctx.lineTo(20,10);\n\
		ctx.lineTo(17,18);\n\
		ctx.lineTo(23,18);\n\
		ctx.lineTo(20,10);\n\
		ctx.stroke();\n\
		for (var i=1; i<=24; i++)\n\
		{\n\
			ctx.fillText(i-1, 15 + 18*i, 295);\n\
		}\n\
	}"
#endif