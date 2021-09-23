#ifndef NVMEWORKER_H
#define NVMEWORKER_H

#include <QObject>
#include <QThread>
#include <QDebug>
#include <QProcess>
#include <QTextStream>
#include <QFile>
#include <QMap>


class nvmeworker : public QObject
{
    Q_OBJECT
public:
    explicit nvmeworker(QObject *parent = nullptr);
    bool done = false;

public slots:
    void doWork();
    void finish();

signals:
    void resultReady(const QString &result);
    void finished();
    void error(QString err);

private:
    int start_tracing();
    int stop_tracing();
    void process_setup(QString * line);
    void process_complete(QString * line);
    QString * line_common(QString * line);
    QStringList fields;
    QMap<int,QMap<int,int>> queues;

};

#endif // NVMEWORKER_H
