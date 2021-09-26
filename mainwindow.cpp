#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->action_Start->setEnabled(false);
    ui->actionS_top->setEnabled(false);
}

MainWindow::~MainWindow()
{
//    if (workerThread->isRunning()  )
//    {
//        workerThread->quit();
//        workerThread->wait(1);
//    }
    delete ui;
}

void MainWindow::errorString(QString err){
    qInfo() << "received error from thread: " << err;
}

void MainWindow::on_actionE_xit_triggered()
{
    QApplication::exit();
}

QStringList MainWindow::toStringList(const QByteArray list) {
    QStringList strings;
    QString inp = QString(list);
    inp.replace("\t","");
    strings = inp.split("\n");
    strings = strings.filter("proc");
    strings = strings.replaceInStrings("processor: ","CPU:");
    return strings;
}

int MainWindow::setup_CPU_selector()
{
    int status = 0;
    QProcess process_system;
    QString my_formatted_string = QString("Running on %1  %2").arg(QSysInfo::currentCpuArchitecture(),QSysInfo::kernelVersion());
    ui->plainTextEdit->appendPlainText(my_formatted_string);
    if(QSysInfo::kernelType() == "linux")
        {
            QStringList linuxcpuname = {"-c"," cat /proc/cpuinfo | grep 'model name' | uniq" };
            process_system.start("bash", linuxcpuname);
            process_system.waitForFinished(3000);
            QByteArray stdout = process_system.readAllStandardOutput();
            qInfo() << "stdout size " << stdout.size();
            QByteArray stderr = process_system.readAllStandardError();
            qInfo() << "stderr size " << stderr.size();
            QString linuxOutput = QString(stdout);
            qInfo() << process_system.exitStatus();
            qInfo() << stdout;
            qInfo() << stderr;
            ui->plainTextEdit->appendPlainText(linuxOutput);

            linuxcpuname = {"-c"," cat /proc/cpuinfo | grep 'processor' " };
            process_system.start("bash", linuxcpuname);
            process_system.waitForFinished(3000);
            stdout = process_system.readAllStandardOutput();
            QStringList cpus = toStringList(stdout);
            qInfo() << cpus;
            QStringList::const_iterator constIterator;
            int i = 0;
            for (constIterator = cpus.constBegin(); constIterator != cpus.constEnd(); ++constIterator)
            {
                QCheckBox * cpu_chx = new QCheckBox();
                cpu_chx->setCheckState(Qt::Checked);
                cpu_boxes.insert(cpu_boxes.end(),cpu_chx);
                ui->cpu_formLayout->addRow(*constIterator,cpu_boxes[i]);
                i++;
            }

        }
    return status;
}

void MainWindow::setup_display()
{
    int num_drives = 4;
    int num_queues_per_drive = 12;
    running = false;

    QPixmap *green_disk = new QPixmap(":/new/mainimages/hdd-green.png");
    for (int i = 0; i < num_drives; i++) {
        qInfo() << "initializing disk " << i;
        HDD * thisDisk = new HDD();
        thisDisk->ndx = i;
        thisDisk->num_queues = num_queues_per_drive;
        thisDisk->name=std::string("Disk");
        thisDisk->name.append(std::to_string(i));
        for (int j = 0; j < num_queues_per_drive; j++) {
            QProgressBar * pBar = new QProgressBar();
            pBar->setTextVisible(false);
            //pBar->setMaximumWidth(200);
            thisDisk->pBars.insert(thisDisk->pBars.end(),pBar);
        }
        thisDisk->hddLayout = new QHBoxLayout();
        thisDisk->disk_frame = new QFrame();
        thisDisk->disk_frame->setFrameStyle(QFrame::Box|QFrame::Raised);
        thisDisk->disk_frame->setLineWidth(3);

        QLabel * ndx = new QLabel();
        ndx->setNum(thisDisk->ndx);
        thisDisk->hddLayout->addWidget(ndx);
        QLabel * icon = new QLabel();
        icon->setPixmap(*green_disk);
        thisDisk->hddLayout->addWidget(icon);
        QVBoxLayout * queues = new QVBoxLayout();
        thisDisk->hddLayout->addLayout(queues);
        for (QProgressBar* qpb : thisDisk->pBars){
            queues->addWidget(qpb);
        }
        thisDisk->statsLayout = new QVBoxLayout();
        thisDisk->bw_label = new QLabel("mbps");
        thisDisk->iops_label = new QLabel("iops");
        thisDisk->hddLayout->addLayout(thisDisk->statsLayout);
        thisDisk->statsLayout->addWidget(thisDisk->bw_label);
        thisDisk->statsLayout->addWidget(thisDisk->iops_label);
        disks.insert(disks.end(),thisDisk);
    }
    qInfo() << disks;

    for (HDD *h : disks) {
        ui->drive_verticalLayout->addWidget(h->disk_frame);
        ui->drive_verticalLayout->addLayout(h->hddLayout);
    }
    ui->plainTextEdit->setStyleSheet("QPlainTextEdit {background-color: black; color: red;}");
    setup_CPU_selector();
    ui->action_Start->setEnabled(false);
    ui->plainTextEdit->appendPlainText("Initialized");
}

void MainWindow::timerEvent(QTimerEvent *event)
{
    ui->statusbar->showMessage("Timer...",400);
}

void MainWindow::on_action_Init_triggered()
{
    setup_display();
    ui->action_Start->setEnabled(true);
    ui->statusbar->showMessage("initializing...");
}

//void MainWindow::handleResults(const QString & result)
//{
//    //qInfo() << "result: " << result;
//    QStringList fields = result.split("|");
//    int drive = fields[0].split(":")[1].toInt();

//    for (int i = 1; i < fields.size(); ++i)
//    {
//        QStringList temp = fields.at(i).split("=");
//        int q = temp[0].toInt();
//        int d = temp[1].toInt();
//    }
//}

void MainWindow::handleResults(const QMap<int, QMap<int,int>> result)
{
    QMapIterator<int, QMap<int,int>> i(result);
    while (i.hasNext())
    {
     i.next();
     qInfo() << "disk:" << i.key();
     QMapIterator<int,int> q(i.value());
     while (q.hasNext())
     {
        q.next();
        //qInfo() << "q:" << q.key() << ":" << q.value();
        disks[i.key()]->pBars[q.key()]->setValue(q.value());
     }
    }
}

void MainWindow::on_action_Start_triggered()
{
    if (running) return;
    running = true;
    qInfo() << "menu start";
    ui->statusbar->showMessage("Starting...",2500);
    //timer = startTimer(1000);
    active_cpus.clear();
    QString iostr;
    for (int i = 0; i < cpu_boxes.size(); i++)
    {
        qInfo() << cpu_boxes[i]->checkState();
        if (cpu_boxes[i]->checkState() == Qt::Checked)
        {
            active_cpus.insert(active_cpus.end(),i);
            iostr.append(QString("CPU:%1 ").arg(i));
        }

    }
    qInfo() << iostr;
    ui->plainTextEdit->appendPlainText(iostr);
    workerThread = new QThread();
    nvmeworker * worker = new nvmeworker();
    worker->moveToThread(workerThread);
    connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    connect(workerThread, SIGNAL(finished()), workerThread, SLOT(deleteLater()));
    connect(workerThread, SIGNAL(started()), worker, SLOT(doWork())); // start working automatically
    connect(worker, SIGNAL(error(QString)), this, SLOT(errorString(QString))); //propagate errors
    connect(worker, SIGNAL(finished()), workerThread, SLOT(quit())); // when worker emits finished it quit()'s thread
    //connect(this, &MainWindow::operate, worker, &nvmeworker::doWork);
    connect(this, SIGNAL(finish_thread()), worker, SLOT(finish()));
    connect(worker, &nvmeworker::resultReady, this, &MainWindow::handleResults);
    workerThread->start();
    ui->statusbar->showMessage("Thread started",1000);
    ui->action_Start->setEnabled(false);
    ui->actionS_top->setEnabled(true);
}

void MainWindow::on_actionS_top_triggered()
{
    if (!running) return;
    running = false;
    ui->action_Start->setEnabled(true);
    ui->actionS_top->setEnabled(false);
    qInfo() << "menu stop";
    emit finish_thread();
   // workerThread.quit();
   // workerThread.wait();
    ui->statusbar->showMessage("Thread stopped",1000);
}


