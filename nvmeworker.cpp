#include "nvmeworker.h"

bool debug = true;

nvmeworker::nvmeworker(QObject *parent) : QObject(parent)
{
    iops_timer = startTimer(1500);
    queue_timer = startTimer(500);
}

nvmeworker::~nvmeworker()
{
     killTimer(iops_timer);
     killTimer(iops_timer);
}

void nvmeworker::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == queue_timer)
    {
        emit resultReady(queues);
        return;
    }

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    double interval = (now - last_time) /1000.0;
    last_time = now;
    QMap<int, int>::iterator i;
    QMap<int,int>iops_results;
    for (i = io_counts.begin(); i != io_counts.end(); ++i)
    {
        iops_results[i.key()] = (int)(i.value() / interval);
        i.value() = 0;
    }

    QMap<int,int>bw_results;
    for (i = io_sectors.begin(); i != io_sectors.end(); ++i)
    {
        bw_results[i.key()] = (int)(i.value() / interval) * sect_size_KB;
        i.value() = 0;
    }
    emit update_iops(iops_results, bw_results);

    qInfo() << "nvmeworker QTimerEvent fired " << iops_results << bw_results;
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
    int stat;
    stat = start_tracing();
    QFile inf("/sys/kernel/debug/tracing/trace_pipe");
    if (!inf.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qInfo() << "unable to open trace file /sys/kernel/debug/tracing/trace_pipe";
        QThread::sleep(1);
    }
    else
    {
        qInfo() << "opened " << inf.fileName();
        QTextStream trace (&inf);
        qInfo() << "opened trace QTextStream";
        while (!done)
        {
            bool yn = trace.readLineInto(&line, 255);
            QCoreApplication::processEvents();
            if (!line.startsWith("#"))
            {
                if (line.contains("nvme_setup_cmd")) process_setup(&line);
                else if (line.contains("nvme_complete_rq")) process_complete(&line);
            }
        }
    }
    stat = stop_tracing();
    queues.clear();
}


int nvmeworker::get_value(QStringList fields, QString selector)
{
    QStringList results = fields.filter(selector);
    if (results.size() == 0)
    {
        qInfo() << "selector not found";
        return 0;
    }
    if (debug) logstream << "get_value " << selector << results[0] << Qt::endl;
    if (debug) logstream.flush();
    return results[0].split("=")[1].toInt();
}

void nvmeworker::process_setup(QString * line)
{
    fields = line->split(":");
    int diskNo = fields[2].simplified().replace("[^0-9]", "").toInt();
    fields = line->replace(":",",").split(",");
    int qid = get_value(fields,"qid");
    int cid = get_value(fields,"cmdid");
    int len = 1 + get_value(fields,"len");
    io_sectors[diskNo] = io_sectors[diskNo] + len;
    //qInfo() << "len:" << len;
    queues[diskNo][qid] = queues[diskNo][qid] + 1;
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
    io_counts[diskNo] += 1;
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
    qInfo() << "worker stdout: " << stdout;
    qInfo() << "worker stderr: " << stderr;
    return stat;
}
