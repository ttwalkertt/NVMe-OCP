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

    void resultReady(const QMap<int, QMap<int,int>> result);
    void finished();
    void update_iops(const QMap<int,int>, const QMap<int,int>);
    void error(QString err);

private:
    int start_tracing();
    int stop_tracing();
    void process_setup(QString * line);
    void process_complete(QString * line);
    QString * line_common(QString * line);
    int get_value(QStringList fields, QString selector);
    QStringList fields;
    QFile * logfile;
    QTextStream  logstream;
    QMap<int,QMap<int,int>> queues;
    QMap<int,int> io_counts;
    QMap<int,int> io_sectors;
    int iops_timer;
    int queue_timer;
    qint64 last_time = 0;
    int sect_size_KB = 4;
    bool active = true;

};

#endif // NVMEWORKER_H
