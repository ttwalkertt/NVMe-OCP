#include "nvmeworker.h"

bool debug = true;

nvmeworker::nvmeworker(QObject *parent) : QObject(parent)
{
    iops_timer = startTimer(1500);
    queue_timer = startTimer(500);
    qInfo() << "timers started";
}

nvmeworker::~nvmeworker()
{
     killTimer(iops_timer);
     killTimer(queue_timer);
}

void nvmeworker::reduce_queue()
{
    /*
    QMapIterator<int, QMap<int,int>> disk(queues);
    while (disk.hasNext())
    {
     disk.next();
     qInfo() << "cleaning queues";
     QMapIterator<int,int> q(disk.value());
     while (q.hasNext())
     {
        q.next();
        if (queues[disk.key()][q.key()] > 1)
        {
            queues[disk.key()][q.key()]--;
            qInfo() << "disk:" << disk.key() << "queue:" << q.key() << "decremented to " << queues[disk.key()][q.key()];
        }
     }
    }
    */
}

void nvmeworker::timerEvent(QTimerEvent *event)
{
    //qInfo() << "timer fired in NVMeworker";
    if (done) return;
    if (event->timerId() == queue_timer)
    {
        qInfo() << "nvmeworker QTimerEvent fired for queues" << queues ;
        if (!active)
        {
            queues.clear();
            qInfo() << "cleared queues counter in NVMe worker";
        }
        emit resultReady(queues);      
        active = false;
        reduce_queue();
        return;
    }
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    double interval = (now - last_time) /1000.0;
    last_time = now;
    QMap<int, int>::iterator i;

    DeviceCounters iops_results;
    QStringList drives = io_counts.keys();
    for (const QString drive : drives)
    {
        iops_results[drive][1] = io_counts[drive][1]/interval;
        iops_results[drive][2] = io_counts[drive][2]/interval;
        io_counts[drive][1] = 0;
        io_counts[drive][2] = 0;
    }

    DeviceCounters bw_results;
    drives = io_sectors.keys();
    for (const QString drive : drives)
    {
        bw_results[drive][1] = io_sectors[drive][1]/interval;
        bw_results[drive][2] = io_sectors[drive][2]/interval;
        io_sectors[drive][1] = 0;
        io_sectors[drive][2] = 0;
    }


    emit update_iops(iops_results, bw_results);
    qInfo() << "nvmeworker QTimerEvent fired. iops:" << iops_results << " bw:" << bw_results;

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
            QCoreApplication::processEvents();
            if (true)
            {
                bool yn = trace.readLineInto(&line, 255);
                if (!line.startsWith("#"))
                {
                    if (line.contains("nvme_setup_cmd")) process_setup(&line);
                    else if (line.contains("nvme_complete_rq")) process_complete(&line);
                }
            }
           else
            {
                qInfo() << "no trace data";
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
        qInfo() << "selector not found:" << selector << fields;
        return 0;
    }
    if (debug) logstream << "get_value " << selector << results[0] << "\n";
    if (debug) logstream.flush();
    return results[0].split("=")[1].toInt();
}

void nvmeworker::get_value(QStringList fields, QString selector, QString*value)
{
    QStringList results = fields.filter(selector);
    if (results.size() == 0)
    {
        qInfo() << "selector not found:" << selector << fields;
        return ;
    }
    if (debug) logstream << "get_value " << selector << results[0] << "\n";
    if (debug) logstream.flush();
    value = &results[0];
}

void nvmeworker::process_setup(QString * line)
{
    active = true;
    fields = line->replace(":",",").split(",");
    QString dev_name = fields[2].simplified();
    if (!dev_name.contains("nvme") || dev_name.contains("_"))
    {
        qInfo() << "setup" << dev_name << fields;
        dev_name = fields[3].simplified();
    }
    //get_value(fields,"disk",&nsname);
    int nsid = get_value(fields,"nsid");
    int qid = get_value(fields,"qid");
    int cid = get_value(fields,"cmdid");
    int len = 1 + get_value(fields,"len");
    io_sectors[dev_name][nsid] = io_sectors[dev_name][nsid] + len;
    //qInfo() << "len:" << len;
    queues[dev_name][nsid][qid] = queues[dev_name][nsid][qid] + 1;
    if (debug) logstream << "setup    disk:" << dev_name << " namespace:" << nsid << " q:" << qid << " cmd:" << cid << " " << queues[dev_name][nsid][qid] << " " << *line  <<"\n";
    //if (debug) qInfo() << dev_name << "fields: " << fields;
}

void nvmeworker::process_complete(QString * line)
{
    if (done) return;
    fields = line->replace(":",",").split(",");
    QString dev_name = fields[2].simplified();
    if (!dev_name.contains("nvme") || dev_name.contains("_"))
    {
        qInfo() << "complete" << dev_name << fields;
        dev_name = fields[3].simplified();
    }
    //qInfo()  <<  dev_name << fields;
    int qid = get_value(fields,"qid");
    int cid = get_value(fields,"cmdid");
    //int nsid = get_value(fields,"nsid");
    int nsid = 1;
    //get_value(fields,"disk",&dev_name);
    queues[dev_name][nsid][qid] = queues[dev_name][nsid][qid] - 1;
    io_counts[dev_name][nsid] += 1;
    if (queues[dev_name][nsid][qid] < 0) queues[dev_name][nsid][qid] = 0;
    if (queues[dev_name][nsid][qid] > 1000) queues[dev_name][nsid][qid] = 1;  //this is a shameless hack to fix an error I don't understand
    if (debug) logstream << "complete disk:" << dev_name << " namespace:" << nsid << " q:" << qid << " cmd:" << cid << " " << queues[dev_name][nsid][qid] << " " << *line << "\n";
    //if (debug) qInfo() << "fields: " << fields;
}

void nvmeworker::finish()
{
    done = true;
    qInfo() << "nvmeworker wants to finish";


}

int nvmeworker::start_tracing()
{
    int stat = 0;
    done=false;
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
    QThread::sleep(1);
    return stat;
}

int nvmeworker::stop_tracing()
{
    int stat = 0;
    done=true;
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

void nvmeworker::reset()
{
    qInfo() << "nvmeworker recieved reset";
    queues.clear();
}
