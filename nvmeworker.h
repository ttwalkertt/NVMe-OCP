#ifndef NVMEWORKER_H
#define NVMEWORKER_H
#include <QCoreApplication>
#include <QObject>
#include <QThread>
#include <QDebug>
#include <QProcess>
#include <QTextStream>
#include <QFile>
#include <QMap>
#include <QTimer>
#include <QDateTime>
#include <QRegularExpression>
#include <QMetaType>

//typedef QMap<QString,QMap<int,QMap<int,int>>> Queues;
//typedef QMap<QString,QMap<int,int>> DeviceCounters;
#define DeviceCounters QMap<QString,QMap<int,int>>
#define Queues QMap<QString,QMap<int,QMap<int,int>>>

class nvmeworker : public QObject
{
    Q_OBJECT
public:
    explicit nvmeworker(QObject *parent = nullptr);
    ~nvmeworker();
    bool done = false;

public slots:
    void doWork();
    void finish();
    void timerEvent(QTimerEvent *event);
    void reset();

signals:

    void resultReady(const Queues);
    void finished();
    void update_iops(const DeviceCounters, const DeviceCounters);
    void error(QString err);

private:
    int start_tracing();
    int stop_tracing();
    void process_setup(QString * line);
    void process_complete(QString * line);
    void reduce_queue();
    QString * line_common(QString * line);
    int get_value(QStringList fields, QString selector);
    void get_value(QStringList fields, QString selector, QString* value);
    QStringList fields;
    QFile * logfile;
    QTextStream  logstream;
    Queues queues;
    DeviceCounters io_counts;
    DeviceCounters io_sectors;
    int iops_timer;
    int queue_timer;
    qint64 last_time = 0;
    int sect_size_KB = 4;
    bool active = true;

};

#endif // NVMEWORKER_H
