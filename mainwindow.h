#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QFormLayout>
#include <QSysInfo>
#include <QTimer>
#include <QProcess>
#include <QRadioButton>
#include <QCheckBox>
#include <QRegularExpression>
#include <QThread>

#include "nvmeworker.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionE_xit_triggered();
    void on_action_Init_triggered();
    void on_action_Start_triggered();
    void on_actionS_top_triggered();
    void handleResults(const QMap<int, QMap<int,int>> result);
    void handleUpdateIOPS(const QMap<int,int> iops_map, const QMap<int,int> bw_map);
    void errorString(QString err);

signals:
    void operate(const QString &);
    void finish_thread();

protected:
    void timerEvent(QTimerEvent *event) override;

private:
    Ui::MainWindow *ui;
    int num_disks = 12;
    int num_cpus = 0;
    struct HDD {
        int ndx;
        int num_queues;
        int bw;
        int iops;
        QString name;
        std::vector<QProgressBar*> pBars;
        QHBoxLayout *hddLayout;
        QVBoxLayout *statsLayout;
        QLabel *iops_label;
        QLabel *bw_label;
        QFrame *disk_frame;
    };
    std::vector<HDD*> disks;
    std::vector<QCheckBox*> cpu_boxes;
    std::vector<int> active_cpus;
    QStringList disks_present;
    QPlainTextEdit* mainTextWindow;
    bool running;
    int timer;
//thread for capturing trace output and sending it to the main thread
    QThread *workerThread;
// thread to run fio
    QThread * fioThread;

// class methods
    void setup_display();
    QStringList find_disks();
    QStringList toStringList(const QByteArray list);
    int setup_CPU_selector();
    void start_fio();

};
#endif // MAINWINDOW_H
