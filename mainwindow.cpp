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
    qRegisterMetaType<QMap<int,QMap<int,int>>>();
    setup_display();
    qApp->setStyleSheet("QGroupBox {  border: 5px solid gray;}");
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
    ui->statusbar->showMessage(my_formatted_string);
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
            qInfo() << linuxOutput;

            QStringList linuxcpuname2 = {"-c"," cat /proc/cpuinfo | grep 'processor' " };
            process_system.start("bash", linuxcpuname2);
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
    qInfo() << "stdout len: " << stdout.size();
    QByteArray stderr = process_system.readAllStandardError();
    qInfo() << "stderr len: " << stderr.size();
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
    qInfo() << "done find_disks()";
    //num_disks = disks.size();
    return disks;
}

void MainWindow::update_qgraph(HDD* disk, std::vector<int> qds)
{

    qreal maxY = 200;
    for (int i = 0; i < qds.size(); i++)
    {
        QRectF r = disk->qBars[i]->rect();
        r.setTop(maxY * (1-qds[i]/full_scale_qd));
    }


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

        switch (i) {
        case 0:
            thisDisk->my_group = ui->groupBox_d1;
            thisDisk->my_view = ui->graphicsView_01;
            break;
        case 1:
            thisDisk->my_group = ui->groupBox_d2;
            thisDisk->my_view = ui->graphicsView_02;
            break;
        case 2:
            thisDisk->my_group = ui->groupBox_d3;
            thisDisk->my_view = ui->graphicsView_03u;
            break;
        case 3:
            thisDisk->my_group = ui->groupBox_d4;
            thisDisk->my_view = ui->graphicsView_04;
            break;
        case 4:
            thisDisk->my_group = ui->groupBox_d5;
            thisDisk->my_view = ui->graphicsView_05;
            break;
        case 5:
            thisDisk->my_group = ui->groupBox_d6;
            thisDisk->my_view = ui->graphicsView_06u;
            break;
        case 6:
            thisDisk->my_group = ui->groupBox_d7;
            thisDisk->my_view = ui->graphicsView_07;
            break;
        case 7:
            thisDisk->my_group = ui->groupBox_d8;
            thisDisk->my_view = ui->graphicsView_08;
            break;
        case 8:
            thisDisk->my_group = ui->groupBox_d9;
            thisDisk->my_view = ui->graphicsView_09u;
            break;
        case 9:
            thisDisk->my_group = ui->groupBox_d10;
            thisDisk->my_view = ui->graphicsView_10;
            break;
        case 10:
            thisDisk->my_group = ui->groupBox_d11;
            thisDisk->my_view = ui->graphicsView_11;
            break;
        case 11:
            thisDisk->my_group = ui->groupBox_d12;
            thisDisk->my_view = ui->graphicsView_12u;
            break;
        default:
            qInfo() << "Error here parsing disk - UI mapping";
            break;
        }
        thisDisk->my_view->setMinimumHeight(250);
        thisDisk->my_scene = new QGraphicsScene();
        thisDisk->my_view->setScene(thisDisk->my_scene);
        thisDisk->ndx = i;
        thisDisk->num_queues = num_cpus + 1;
        qreal w = thisDisk->my_view->width();
        qreal h = thisDisk->my_view->height();
        qInfo() << "disk:" << i << "view heigth:width" << h << w;
        qreal bar_width = w / (thisDisk->num_queues + 2);
        int bar_margin = 3;
        for (int i = 0; i < thisDisk->num_queues; i++)
        {
            QGraphicsRectItem * r = new QGraphicsRectItem(i*bar_width + bar_margin,0,bar_width,h);
            r->setBrush(QBrush(Qt::red));
            qInfo() << "bar: X:" << i*bar_width << "width: " << bar_width << "heigth: " <<  h;
            thisDisk->my_scene->addItem(r);
        }
        thisDisk->lastQD = new std::vector<int>;
        for (int j = 0; j < thisDisk->num_queues; j++) {
            // init last qd to 0
            thisDisk->lastQD->insert(thisDisk->lastQD->end(),0);
            // create a progress bar for each q
            QProgressBar * pBar = new QProgressBar();
            pBar->setMaximumHeight(10);
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
        thisDisk->selector_checkbox = new QCheckBox();
        QLabel * cb_label;
        if (thisDisk->present)
        {
            thisDisk->selector_checkbox->setCheckState(Qt::Checked);
            cb_label = new QLabel("enabled");
            qInfo() << thisDisk->name;
        } else
        {
            thisDisk->selector_checkbox->setCheckState(Qt::Unchecked);
            cb_label = new QLabel("<s>enabled</s>");
            thisDisk->selector_checkbox->setEnabled(false);
        }
        QFormLayout *formlayout_TGT_int =  new QFormLayout();
        //QLabel * cb_label = new QLabel("enabled");
        formlayout_TGT_int->addRow(cb_label,thisDisk->selector_checkbox);
        icons->addLayout(formlayout_TGT_int);
        thisDisk->hddLayout->addLayout(icons);
        QVBoxLayout * queues = new QVBoxLayout();
        thisDisk->hddLayout->addLayout(queues);
        QLabel *ql = new QLabel("queues");
        ql->setAlignment(Qt::AlignCenter);
        queues->addWidget(ql);
        QFont f( "Arial", 10);
        int c = 0;
        for (QProgressBar* qpb : thisDisk->pBars){
            QHBoxLayout *h = new QHBoxLayout();
            queues->addLayout(h);
            QLabel * qn = new QLabel(QString::number(c));
            qn->setFont(f);
            h->addWidget(qn);
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
        qInfo() << "tracking disk " << thisDisk->name;
        disks.insert(disks.end(),thisDisk);
    }
    qInfo() << disks;

    int c = 0;
    qInfo() << "scanning for NVMe disks";
    for (HDD *h : disks) {
//        h->selector_checkbox = new QCheckBox();
//        if (h->present)
//        {
//            h->selector_checkbox->setCheckState(Qt::Checked);
//            ui->plainTextEdit->appendPlainText(h->name);
//        } else
//        {
//            h->selector_checkbox->setCheckState(Qt::Unchecked);
//            h->selector_checkbox->setEnabled(false);
//        }
        qInfo() << "adding disk " << h->name << "to layout";
//        ui->CPUformLayout_Target->addRow(h->name,h->selector_checkbox);
        if (c < 3)
        {
            //ui->drive_verticalLayout->addWidget(h->disk_frame);
            //ui->drive_verticalLayout->addLayout(h->hddLayout);
        } else if (c < 6)
        {
            //ui->drive_verticalLayout2->addWidget(h->disk_frame);
            //ui->drive_verticalLayout2->addLayout(h->hddLayout);
        } else if (c < 9)
        {
            //ui->drive_verticalLayout3->addWidget(h->disk_frame);
            //ui->drive_verticalLayout3->addLayout(h->hddLayout);
        }
        else
        {
            //ui->drive_verticalLayout4->addWidget(h->disk_frame);
            //ui->drive_verticalLayout4->addLayout(h->hddLayout);
         }
        c++;
    }
    ui->plainTextEdit->setStyleSheet("QPlainTextEdit {background-color: black; color: red;}");
    ui->action_Start->setEnabled(false);
    ui->StopWork->setEnabled(false);
    qInfo() << "done setup_display()";
    qInfo() << "stat: Initialized";
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
    read_chassis_serialport();
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
    read_chassis_serialport();
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
    QMapIterator<int, QMap<int,int>> disk(result);
    while (disk.hasNext())
    {
     disk.next();
     qInfo() << "QD data" << disk.value();
     QMapIterator<int,int> q(disk.value());
     while (q.hasNext())
     {
        q.next();
        update_qgraph(disks[disk.key()],q.value());
        if (q.key() < disks[disk.key()]->pBars.size())
        {
            disks[disk.key()]->pBars[q.key()]->setValue(q.value());
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
    connect(worker, &nvmeworker::update_chassis, this, &MainWindow::handleUpdateChassisPort);
    workerThread->start();
    ui->statusbar->showMessage("Thread started",1000);
    ui->SetWork->setEnabled(true);
    qInfo() << "done start_system()";
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
    qInfo() << "fio stopped";
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
            qInfo() << (QString("added %1").arg(d->name));
        }
    }
    qInfo() << "fio targets:" << targets;
    qInfo() << "done get_fio_targets()" ;
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
    connect(fioProcess,SIGNAL(readyReadStandardError()),this,SLOT(on_fio_stdout()));
    fioProcess->start(program,fioParameters);
    QThread::msleep(250);
    qInfo() << "waiting for fio to start..." << fioProcess->state();
    fio_need_killing = true;
    qInfo() << "done start_fio()";
    return status;
}

void MainWindow::on_SetWork_clicked()
{
    if (start_fio())
    {
        ui->statusbar->showMessage("unable to start fio, please check things out and try again");
        QMessageBox msgBox;
        msgBox.setText("unable to start fio, please check things out and try again");
        msgBox.exec();
        return;
    }
    update_ok = true;
    ui->SetWork->setEnabled(false);
    qInfo() << "done on_SetWork_clicked()";
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
    qInfo() << "done on_StopWork_clicked()";
}

bool MainWindow::setup_chassis_serialport()
{
    bool stat = true;
    QStringList portnames;
    QList<QSerialPortInfo> portinfos = QSerialPortInfo::availablePorts();
    for (QSerialPortInfo p : portinfos)
    {
        portnames << p.portName();
    }
    QString port = QInputDialog::getItem(this, "Select port", "port:", portnames);
    qInfo() << "selected port" << port;
    chassis_port = new QSerialPort();
    chassis_port->setPortName(port);
    chassis_port->setDataBits(QSerialPort::Data8);
    chassis_port->setParity(QSerialPort::NoParity);
    chassis_port->setStopBits(QSerialPort::OneStop);
    chassis_port->setBaudRate(230400);
    chassis_port->setFlowControl(QSerialPort::NoFlowControl);
    chassis_port->open(QIODevice::ReadOnly);
    read_chassis_serialport();
    chassis_timer = startTimer(500);
    chassis_port_up = true;
    qInfo() << "done setup_chassis_serialport()";
    return stat;
}

void MainWindow::read_chassis_serialport()
{
    //qInfo() << "read chassis port, port is "  << chassis_port_up;
    int ctr = 0;
    char buffer[512];
    if (chassis_port_up)
    {
        while ( chassis_port->canReadLine())
        {
            chassis_port->readLine(buffer,511);
            qInfo() << buffer;
            ui->plainTextEdit->appendPlainText(buffer);
            ctr++;
            if (ctr > 5) return;
        }
    }
}

void MainWindow::handleUpdateChassisPort()
{
    read_chassis_serialport();
}

void MainWindow::on_action_Serial_Setup_triggered()
{
    bool stat = setup_chassis_serialport();
    qInfo() << "done on_action_Serial_Setup_triggered()";
}

