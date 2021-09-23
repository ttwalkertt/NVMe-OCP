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
    void handleResults(const QString &);
    void errorString(QString err);
signals:
    void operate(const QString &);
    void finish_thread();

protected:
    void timerEvent(QTimerEvent *event) override;
private:
    Ui::MainWindow *ui;
    void setup_display();
    struct HDD {
        int ndx;
        int num_queues;
        int bw;
        int iops;
        std::string name;
        std::vector<QProgressBar*> pBars;
        QHBoxLayout *hddLayout;
        QVBoxLayout *statsLayout;
        QLabel *iops_label;
        QLabel *bw_label;
        QFrame *disk_frame;
    };
    std::vector<HDD*> disks;
    std::vector<QCheckBox*> cpu_boxes;
    QPlainTextEdit* mainTextWindow;
    int setup_CPU_selector();
    int run_queues();
    QStringList toStringList(const QByteArray list);
    int timer;
    std::vector<int> active_cpus;
    QThread *workerThread;
    bool running;

};
#endif // MAINWINDOW_H
