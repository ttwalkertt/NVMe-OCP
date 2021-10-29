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
    void handleResults(const Queues);
    void handleUpdateIOPS(const DeviceCounters iops_map, const DeviceCounters bw_map);
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
    bool done;
    // 2U12 support
    int num_disks = 12;

    //compute platform support
    int num_cpus = 0;

    //INI file support
    QString iniFileName = "OCP_settings.ini";
    void load_INI_settings();
    int num_chassis_slots = 12;
    std::map<int,QString> INI_slot_to_drive_map;
    QMap<QString,int> drive_to_slot_map;

    // QD chart sizing
    // these are defaults and will be loaded from the INI
    qreal qd_chart_maxY = 200;  // height of the qd chart bars in pixels. update this when building the UI
    qreal full_scale_qd = 100;  // full scale queue depth - will cap the display here -
    int drive_top_row_heigth = 10;  //heigth of the top row. overriden by INI
    int drive_graphics_view_heigth = 200; //ibid
    int drive_graphics_view_width = 500;
    int drive_group_box_heigth; // in INI
    int drive_group_box_width = 600; // in INI
    int grid_margin = 15;
    int grid_row_0_min_h = 40;
    int DAdrive_group_box_heigth = 0; // in INI


    struct NS {
        QString name;
        bool present;
    };

    // one of these for each disk
    struct HDD {
        bool present;
        int ndx;
        bool is_DA;
        struct NS namespaces[3];
        int num_queues;
        int bw;
        int iops;
        int slot_no;
        QString name;
        QString full_name;
        std::vector<QGraphicsRectItem *> qBars;
        std::vector<int> * lastQD;
        //QHBoxLayout *hddLayout;
        QVBoxLayout *statsLayout;
        QLabel *iops_label;
        QLabel *bw_label;
        QCheckBox * selector_checkbox;
        QGroupBox * my_slot;
        QGridLayout * my_grid_layout;
        QGraphicsScene * my_scene[2];
        QGraphicsView * my_view[2];
        QVBoxLayout * left_col_layout;
    };
    std::vector<QGroupBox*> my_slots;
    std::vector<HDD*> disks;
    QMap<QString,HDD*> my_disks;
    std::vector<QCheckBox*> cpu_boxes;
    std::vector<int> active_cpus;
    QStringList disks_present;
    QPlainTextEdit* mainTextWindow;
    QFormLayout *formlayout_CPU;

    struct Chassis_slot{
        int ndx;
        struct HDD* disk;
    };
    std::vector<Chassis_slot> my_chassis_slots;

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
    QString get_fio_CPUs();
    void setup_chassis_serialport();
    void read_chassis_serialport();
    void update_qgraph(HDD*, const QMap<int,int> qds);
    void exercise_qd_display();
    QPushButton * pushbutton_resetCPU;

};
#endif // MAINWINDOW_H
