#include "nvmeworker.h"

bool debug = false;

nvmeworker::nvmeworker(QObject *parent) : QObject(parent)
{

}

void nvmeworker::doWork()
{
    QString result;
    QString line;
    logfile = new QFile("log.txt");
    if (logfile->open(QFile::WriteOnly | QFile::Truncate))
    {
        logstream.setDevice(logfile);
    }
    line.reserve(255);
    result = "a result";
    int ctr = 0;
    int stat;
    stat = start_tracing();
    QFile inf("/sys/kernel/debug/tracing/trace_pipe");
    if (!inf.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qInfo() << "unable to open trace file";
        QThread::sleep(1);
    }
    else
    {
        qInfo() << "opened " << inf.fileName();
        QTextStream trace (&inf);
        qInfo() << "opened QTextStream";
        while (!done)
        {
            bool yn = trace.readLineInto(&line, 255);
            //qInfo() << "thread loop " << ctr << " readlininto: " << yn;
            //qInfo() << line;
            //if (debug) logstream << line << "\n" ;
            if (debug) logstream.flush();
            if (!line.startsWith("#"))
            {
                if (line.contains("nvme_setup_cmd")) process_setup(&line);
                else if (line.contains("nvme_complete_rq")) process_complete(&line);
            }

            ctr++;
            if ((ctr % 20) == 0)
            {
                QString qr;
                QStringList outstrs;
                if (debug) logstream << "==============================================================";
                if (debug) logstream << ctr;
//                QMapIterator<int, QMap<int,int>> i(queues);
//                while (i.hasNext())
//                {
//                 i.next();
//                 if (debug) logstream << "drive: " << i.key() << " ";
//                 outstrs.append( QString("d:%1").arg(i.key()));
//                 QMapIterator<int,int> q(i.value());
//                 while (q.hasNext())
//                 {
//                    q.next();
//                    qr=QString("%1=%2").arg(q.key()).arg(q.value());
//                    logstream << qr << Qt::endl;
//                    outstrs.append(qr);
//                    if (debug) logstream << q.key() << ":" << q.value() << " " ;
//                 }
//                 if (debug) logstream << Qt::endl;
                 emit resultReady(queues);

                 //emit resultReady(outstrs.join("|"));
                 logstream << outstrs.join("|") << Qt::endl;
//                }
                logstream.flush();
            }
        }
    }
    stat = stop_tracing();
}

//depreciated
QString * nvmeworker::line_common(QString * line)
{
    return line;
}


int nvmeworker::get_value(QStringList fields, QString selector)
{
    QStringList results = fields.filter(selector);
    if (debug) logstream << "get_value results: " << results[0] << Qt::endl;
    return results[0].split("=")[1].toInt();
}

void nvmeworker::process_setup(QString * line)
{
    fields = line->split(":");
    int diskNo = fields[2].simplified().replace("[^0-9]", "").toInt();
    fields = line->replace(":",",").split(",");
    int qid = get_value(fields,"qid");
    int cid = get_value(fields,"cmdid");
    queues[diskNo][qid] = queues[diskNo][qid] + 1;
    //qInfo() << "setup " << diskNo << " " << qid << cid << queues[diskNo][qid];
    if (debug) logstream << "setup    disk:" << diskNo << " q:" << qid << " c:" << cid << " " << queues[diskNo][qid] << " " << *line <<"\n";
}

void nvmeworker::process_complete(QString * line)
{
    fields = line->split(":");
    int diskNo = fields[2].simplified().replace("[^0-9]", "").toInt();
    fields = line->replace(":",",").split(",");
    int qid = get_value(fields,"qid");
    int cid = get_value(fields,"cmdid");
    queues[diskNo][qid] = queues[diskNo][qid] - 1;
    if (queues[diskNo][qid] < 0) queues[diskNo][qid] = 0;
    if (debug) logstream << "complete disk:" << diskNo << " q:" << qid << " c:" << cid << " " << queues[diskNo][qid] << " " << *line << "\n";
}


void nvmeworker::finish()
{
    done = true;
    qInfo() << "nvmeworker wants to finish";
}

int nvmeworker::start_tracing()
{
    int stat = 0;
    QProcess process_system;
    QStringList linuxcpuname = {"-c"," echo 1 > /sys/kernel/debug/tracing/events/nvme/enable" };
    qInfo() << "worker: " << linuxcpuname;
    process_system.start("bash", linuxcpuname);
    process_system.waitForFinished(3000);
    QByteArray stdout = process_system.readAllStandardOutput();
    qInfo() << "worker stdout size " << stdout.size();
    QByteArray stderr = process_system.readAllStandardError();
    qInfo() << "worker stderr size " << stderr.size();
    QString linuxOutput = QString(stdout);
    qInfo() << process_system.exitStatus();
    qInfo() << "worker: " << stdout;
    qInfo() << "worker: " << stderr;
    QThread::sleep(2);
    return stat;
}

int nvmeworker::stop_tracing()
{
    int stat = 0;
    QProcess process_system;
    QStringList linuxcpuname = {"-c"," echo 0 > /sys/kernel/debug/tracing/events/nvme/enable" };
    qInfo() << "worker: " << linuxcpuname;
    process_system.start("bash", linuxcpuname);
    process_system.waitForFinished(3000);
    QByteArray stdout = process_system.readAllStandardOutput();
    qInfo() << "worker stdout size " << stdout.size();
    QByteArray stderr = process_system.readAllStandardError();
    qInfo() << "worker stderr size " << stderr.size();
    QString linuxOutput = QString(stdout);
    qInfo() << process_system.exitStatus();
    qInfo() << "worker: " << stdout;
    qInfo() << "worker: " << stderr;
    return stat;
}
