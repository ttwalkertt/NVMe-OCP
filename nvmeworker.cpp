#include "nvmeworker.h"

nvmeworker::nvmeworker(QObject *parent) : QObject(parent)
{

}

void nvmeworker::doWork()
{
    QString result;
    QString line;
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
            qInfo() << "thread loop " << ctr << " readlininto: " << yn;
            //qInfo() << line;
            /* ... here is the expensive or blocking operation ... */
            if (!line.startsWith("#"))
            {
                if (line.contains("nvme_setup_cmd")) process_setup(&line);
                else if (line.contains("nvme_complete_rq")) process_complete(&line);
            }
            emit resultReady(result);
            ctr++;
            if (ctr > 50) break;
        }
    }
    stat = stop_tracing();
}

//depreciated
QString * nvmeworker::line_common(QString * line)
{
    return line;
}


void nvmeworker::process_setup(QString * line)
{
    fields = line->split(":");
    int diskNo = fields[2].simplified().replace("[^0-9]", "").toInt();
    fields = line->replace(":",",").split(",");
    int qid = fields.filter("qid")[0].split("=")[1].toInt();
    int cid = fields.filter("cmdid")[0].split("=")[1].toInt();
    qInfo() << "setup" << diskNo << qid << cid;
}

void nvmeworker::process_complete(QString * line)
{
    fields = line->split(":");
    int diskNo = fields[2].simplified().replace("[^0-9]", "").toInt();
    fields = line->replace(":",",").split(",");
    int qid = fields.filter("qid")[0].split("=")[1].toInt();
    int cid = fields.filter("cmdid")[0].split("=")[1].toInt();
    qInfo() << "complete" << diskNo << qid << cid;
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
