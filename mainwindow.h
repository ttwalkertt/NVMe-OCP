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
#include <QMessageBox>
#include <QInputDialog>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QGroupBox>
#include <QSettings>
#include <QFileInfo>


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
    void handleUpdateChassisPort();
    void errorString(QString err);
    void on_SetWork_clicked();
    void on_fio_exit(int exitCode, QProcess::ExitStatus exitStatus);
    void on_fio_stdout();
    void on_StopWork_clicked();
    void on_action_Serial_Setup_triggered();
    void on_pushButton_clicked();

signals:
    void operate(const QString &);
    void finish_thread();
    void reset_nvmeworker();

protected:
    void timerEvent(QTimerEvent *event) override;

private:
    Ui::MainWindow *ui;
    // 2U12 support
    int num_disks = 12;

    //compute platform support
    int num_cpus = 0;

    //INI file support
    QString iniFileName = "OCP_settings.ini";
    void load_INI_settings();
    std::map<int,QString> INI_my_slots;

    // QD chart sizing
    qreal qd_chart_maxY = 200;  // height of the qd chart bars in pixels. update this when building the UI
    qreal full_scale_qd = 100;  // full scale queue depth - will cap the display here
    // one of these for each disk
    struct HDD {
        bool present;
        int ndx;
        int num_queues;
        int bw;
        int iops;
        QString name;
        std::vector<QProgressBar*> pBars;
        std::vector<QGraphicsRectItem *> qBars;
        std::vector<int> * lastQD;
        QHBoxLayout *hddLayout;
        QVBoxLayout *statsLayout;
        QLabel *iops_label;
        QLabel *bw_label;
        QFrame *disk_frame;
        QCheckBox * selector_checkbox;
        QGroupBox * my_group;
        QGraphicsScene * my_scene;
        QGraphicsView * my_view;
    };
    std::vector<QGroupBox*> my_slots;
    std::vector<HDD*> disks;
    std::vector<QCheckBox*> cpu_boxes;
    std::vector<int> active_cpus;
    QStringList disks_present;
    QPlainTextEdit* mainTextWindow;
    QFormLayout *formlayout_CPU;
    bool running;
    bool update_ok = false;
    int timer;
    bool thread_needs_killed = false;
    bool fio_need_killing = false;
//thread for capturing trace output and sending it to the main thread
    QThread *workerThread;
// thread to run fio
    QProcess *fioProcess;

    int dummyqd_input = 0;

// class methods
    void setup_display();
    QStringList find_disks();
    QStringList toStringList(const QByteArray list);
    int setup_CPU_selector();
    int start_fio();
    void start_system();
    QStringList get_fio_targets();
    void setup_chassis_serialport();
    void read_chassis_serialport();
    void update_qgraph(HDD*, QMap<int,int> qds);
    void exercise_qd_display();

};
#endif // MAINWINDOW_H
