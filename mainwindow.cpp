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
    ui->SetWork->setEnabled(false);
    setup_display();
    start_system();

}

MainWindow::~MainWindow()
{
    on_StopWork_clicked();
    if (thread_needs_killed)
    {
        workerThread->quit();
        ui->statusbar->showMessage("shutting down...");
        workerThread->wait(5);
    }
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
            num_cpus = 0;
            formlayout_CPU =  new QFormLayout();
            ui->groupBox_CPU->setLayout(formlayout_CPU);
            for (constIterator = cpus.constBegin(); constIterator != cpus.constEnd(); ++constIterator)
            {
                QCheckBox * cpu_chx = new QCheckBox();
                cpu_chx->setCheckState(Qt::Checked);
                cpu_boxes.insert(cpu_boxes.end(),cpu_chx);
                formlayout_CPU->addRow(*constIterator,cpu_boxes[num_cpus]);
                num_cpus++;
            }
            qInfo() << "num cpus:" << num_cpus;
        }
    return status;
}

QStringList MainWindow::find_disks()
{
    QProcess process_system;
    QStringList disks;
    QStringList linuxnvmelist = {"-c"," nvme list | grep nvme" };
    process_system.start("bash", linuxnvmelist);
    process_system.waitForFinished(3000);
    QByteArray stdout = process_system.readAllStandardOutput();
    qInfo() << "stdout size " << stdout.size();
    QByteArray stderr = process_system.readAllStandardError();
    qInfo() << "stderr size " << stderr.size();
    QStringList temp = QString(stdout).split("\n");
    qInfo() << process_system.exitStatus();
    qInfo() << "stdout: " << stdout;
    qInfo() << "stderr: " << stderr;
    for (int i = 0; i < temp.size(); ++i)
    {
        QString junk = temp.at(i);
        junk = junk.replace("\t"," ");
        if (junk.size() > 2) disks.append(junk.simplified().split(" ")[0]);
    }
    qInfo() << disks;
    //num_disks = disks.size();
    return disks;
}

void MainWindow::setup_display()
{

    disks_present = find_disks();
    running = false;
    setup_CPU_selector();
    QPixmap *green_disk = new QPixmap(":/new/mainimages/hdd-green.png");
    for (int i = 0; i < num_disks; i++) {
        qInfo() << "initializing disk " << i;
        HDD * thisDisk = new HDD();
        if (i < disks_present.size())
        {
            thisDisk->name = disks_present[i].split("/")[2];
            thisDisk->present = true;
        }
        else
        {
            thisDisk->name = QString("not present");
            thisDisk->present = false;
        }
        thisDisk->ndx = i;
        thisDisk->num_queues = num_cpus + 1;
        for (int j = 0; j < thisDisk->num_queues; j++) {
            QProgressBar * pBar = new QProgressBar();
            pBar->setTextVisible(false);
            //pBar->setMaximumWidth(200);
            thisDisk->pBars.insert(thisDisk->pBars.end(),pBar);
        }
        thisDisk->hddLayout = new QHBoxLayout();
        thisDisk->disk_frame = new QFrame();
        thisDisk->disk_frame->setFrameStyle(QFrame::Box|QFrame::Raised);
        thisDisk->disk_frame->setLineWidth(3);
        thisDisk->disk_frame->setLayout(thisDisk->hddLayout);

        QLabel * ndx = new QLabel();
        ndx->setText(thisDisk->name);
        ndx->setAlignment(Qt::AlignHCenter );
        QVBoxLayout* icons = new QVBoxLayout();
        QLabel * icon = new QLabel();
        icon->setPixmap(*green_disk);
        icons->addWidget(icon);
        icons->addWidget(ndx);
        thisDisk->hddLayout->addLayout(icons);
        //thisDisk->hddLayout->addWidget(ndx);
        //thisDisk->hddLayout->addWidget(icon);
        QVBoxLayout * queues = new QVBoxLayout();
        thisDisk->hddLayout->addLayout(queues);
        int c = 0;
        for (QProgressBar* qpb : thisDisk->pBars){
            QHBoxLayout *h = new QHBoxLayout();
            queues->addLayout(h);
            h->addWidget(new QLabel(QString::number(c)));
            h->addWidget(qpb);
            c++;
        }
        thisDisk->statsLayout = new QVBoxLayout();
        thisDisk->bw_label = new QLabel("mbps");
        thisDisk->iops_label = new QLabel("iops");
        thisDisk->hddLayout->addLayout(thisDisk->statsLayout);
        thisDisk->statsLayout->addWidget(new QLabel("KBPS"));
        thisDisk->statsLayout->addWidget(thisDisk->bw_label);
        thisDisk->statsLayout->addWidget(new QLabel("IOPS"));
        thisDisk->statsLayout->addWidget(thisDisk->iops_label);
        disks.insert(disks.end(),thisDisk);
    }
    qInfo() << disks;

    int c = 0;
    ui->plainTextEdit->appendPlainText("scanning for NVMe disks");
    for (HDD *h : disks) {
        h->selector_checkbox = new QCheckBox();
        if (h->present)
        {
            h->selector_checkbox->setCheckState(Qt::Checked);
            ui->plainTextEdit->appendPlainText(h->name);
        } else
        {
            h->selector_checkbox->setCheckState(Qt::Unchecked);
            h->selector_checkbox->setEnabled(false);
        }
        ui->CPUformLayout_Target->addRow(h->name,h->selector_checkbox);
        if (c < 3)
        {
            ui->drive_verticalLayout->addWidget(h->disk_frame);
            ui->drive_verticalLayout->addLayout(h->hddLayout);
        } else if (c < 6)
        {
            ui->drive_verticalLayout2->addWidget(h->disk_frame);
            ui->drive_verticalLayout2->addLayout(h->hddLayout);
        } else if (c < 9)
        {
            ui->drive_verticalLayout3->addWidget(h->disk_frame);
            ui->drive_verticalLayout3->addLayout(h->hddLayout);
        }
        else
        {
            ui->drive_verticalLayout4->addWidget(h->disk_frame);
            ui->drive_verticalLayout4->addLayout(h->hddLayout);
         }
        c++;
    }
    ui->plainTextEdit->setStyleSheet("QPlainTextEdit {background-color: black; color: red;}");
    ui->action_Start->setEnabled(false);
    ui->StopWork->setEnabled(false);
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
    ui->SetWork->setEnabled(true);
    ui->statusbar->showMessage("initializing...");
}

void MainWindow::handleUpdateIOPS(const QMap<int,int> iops_map, const QMap<int,int> bw_map )
{
    if (!update_ok)
    {
        qInfo() << "received IOPS update with display update disabled";
        for(auto e : disks)
        {
          e->bw_label->setNum(0);
          e->iops_label->setNum(0);
        }
        return;
    }
    qInfo() << "main thread received IOPs/ BW data" << iops_map << bw_map;

    QMapIterator<int,int> i(iops_map);
    while (i.hasNext())
    {
        i.next();
        disks[i.key()]->iops_label->setNum(i.value());
    }
    QMapIterator<int,int> j(bw_map);
    while (j.hasNext())
    {
        j.next();
        disks[j.key()]->bw_label->setNum(j.value());
    }
}

void MainWindow::handleResults(const QMap<int, QMap<int,int>> result)
{
    if (!update_ok)
    {
        qInfo() << "received QD update with display update disabled";
        for(auto e : disks)
        {
          for (auto p : e->pBars)
          {
              p->setValue(0);
          }
        }
        return;
    }
    QMapIterator<int, QMap<int,int>> i(result);
    while (i.hasNext())
    {
     i.next();
     qInfo() << "QD data" << i.value();
     QMapIterator<int,int> q(i.value());
     while (q.hasNext())
     {
        q.next();
        if (q.key() < disks[i.key()]->pBars.size())
        {
            disks[i.key()]->pBars[q.key()]->setValue(q.value());
        }
     }
    }
}

void MainWindow::on_action_Start_triggered()
{

}

void MainWindow::start_system()
{
    if (running) return;
    running = true;
    thread_needs_killed = true;
    qInfo() << "menu start";
    ui->statusbar->showMessage("Starting...",2500);
    //timer = startTimer(1000);
    active_cpus.clear();
    QString iostr;
    for (unsigned long i = 0; i < cpu_boxes.size(); i++)
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
    connect(this, SIGNAL(finish_thread()), worker, SLOT(finish()));
    connect(this, SIGNAL(reset_nvmeworker()), worker, SLOT(reset()));
    connect(worker, &nvmeworker::resultReady, this, &MainWindow::handleResults);
    connect(worker, &nvmeworker::update_iops, this, &MainWindow::handleUpdateIOPS);
    workerThread->start();
    ui->statusbar->showMessage("Thread started",1000);
    ui->SetWork->setEnabled(true);
}

void MainWindow::on_actionS_top_triggered()
{
    if (!running) return;
    running = false;
    ui->action_Start->setEnabled(true);
    ui->actionS_top->setEnabled(false);
    qInfo() << "menu stop";
    emit finish_thread();
    ui->statusbar->showMessage("Thread stopped",1000);
}

void MainWindow::on_fio_exit(int exitCode, QProcess::ExitStatus exitStatus)
{
    qInfo() << "recieved fio exit signal" << exitCode << exitStatus;
    ui->statusbar->showMessage("fio stopped");
    ui->plainTextEdit->appendPlainText("fio stopped");
    //delete fioProcess;
}

void MainWindow::on_fio_stdout()
{
    qInfo() << "fio:" << fioProcess->readAllStandardOutput();
    qInfo() << "fio:" << fioProcess->readAllStandardError();
}

QStringList MainWindow::get_fio_targets()
{
    QStringList targets;
    for (auto d : disks)
    {
        if (d->selector_checkbox->checkState() == Qt::Checked)
        {
            targets.append(d->name);
            ui->plainTextEdit->appendPlainText(QString("added %1").arg(d->name));
        }
    }
    qInfo() << "fio targets:" << targets;
    return targets;
}

int MainWindow::start_fio()
{
    int status = 0;
    QStringList targets = get_fio_targets();
    QStringList fioParameters;
    fioParameters.append("--name=global");
    fioParameters.append("--direct=1");
    fioParameters.append("--ioengine=libaio");

    QStringList fioJobParameters;
    if (ui->radioButton4K->isChecked()) fioJobParameters.append("--bs=4k");
    else if (ui->radioButton64K->isChecked()) fioJobParameters.append("--bs=64k");
    else if (ui->radioButton256K->isChecked()) fioJobParameters.append("--bs=256K");
    if (ui->radioButtonRandom->isChecked())
    {
        if (ui->radioButtonRWMixed->isChecked()) fioJobParameters.append("--rw=randrw");
        else if (ui->radioButtonRead->isChecked()) fioJobParameters.append("--rw=randread");
        else if (ui->radioButtonWrite->isChecked()) fioJobParameters.append("--rw=randwrite");
        else qInfo() << "no valid workload";
    } else
    {
        if (ui->radioButtonRWMixed->isChecked()) fioJobParameters.append("--rw=rw");
        else if (ui->radioButtonRead->isChecked()) fioJobParameters.append("--rw=read");
        else if (ui->radioButtonWrite->isChecked()) fioJobParameters.append("--rw=write");
        else qInfo() << "no valid workload";
    }
    QString qd = QString("--iodepth=%1").arg(ui->spinBoxQD->value());
    fioJobParameters.append(qd);
    int disk_test_cnt = 0;
    for (HDD *h : disks)
    {
        if ((h->present) && (h->selector_checkbox->isChecked() == true))
        {
            fioParameters.append(QString("--name=%1").arg(h->ndx));
            fioParameters.append(QString("--filename=/dev/%1").arg(h->name));
            fioParameters.append(fioJobParameters);
            disk_test_cnt++;
        }
    }
    if (!disk_test_cnt)
    {
        QMessageBox msgBox;
        msgBox.setText("No disks are available or selected to run!");
        msgBox.exec();
        return -1;
    }
    ui->statusbar->showMessage("starting FIO workload");
    qInfo() << fioParameters;
    QString program = "fio";
    ui->SetWork->setEnabled(false);
    ui->StopWork->setEnabled(true);
    fioProcess = new QProcess();
    connect(fioProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(on_fio_exit(int, QProcess::ExitStatus)));
    connect(fioProcess,SIGNAL(readyReadStandardOutput()),this,SLOT(on_fio_stdout()));
    fioProcess->start(program,fioParameters);
    int timer = 0;
    QThread::msleep(250);
    qInfo() << "waiting for fio to start..." << fioProcess->state();
    fio_need_killing = true;
    return status;
}

void MainWindow::on_SetWork_clicked()
{
    if (start_fio())
    {
        ui->plainTextEdit->appendPlainText("unable to start fio, please check things out and try again");
        return;
    }
    update_ok = true;
    ui->SetWork->setEnabled(false);
}


void MainWindow::on_StopWork_clicked()
{
    qInfo() << "sending kill to fio";
    if (fio_need_killing)
    {
        fioProcess->terminate();
        QStringList linuxnvmelist = {"-c","pkill" , "fio" };
        QProcess process_system;
        process_system.start("bash", linuxnvmelist);
        process_system.waitForFinished(30000);
        qInfo() << "kill fio " << process_system.exitStatus();
        fio_need_killing = false;
    }
    ui->SetWork->setEnabled(true);
    ui->StopWork->setEnabled(false);
    reset_nvmeworker();
    update_ok = false;
}

