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
    qRegisterMetaType<Queues>();
    qRegisterMetaType<DeviceCounters>();
    qRegisterMetaType<QMap<QString,QMap<int,QMap<int,int>>>>();
    qRegisterMetaType<QMap<QString,QMap<int,int>>>();
    load_INI_settings();
    setup_display();
    qApp->setStyleSheet("QGroupBox {  border: 5px solid gray;}");
    start_system();

}

MainWindow::~MainWindow()
{
    on_StopWork_clicked();
//    if (thread_needs_killed)
//    {
//        workerThread->quit();
//        ui->statusbar->showMessage("shutting down...");
//        workerThread->wait(5);
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

void MainWindow::load_INI_settings()
{
    qInfo() << "loading INI settings from" << iniFileName;
    if(QFileInfo::exists(iniFileName) && QFileInfo(iniFileName).isFile())
    {
        QSettings * settings = new QSettings(iniFileName,QSettings::IniFormat);
        qInfo() << "sections found in INI" << settings->childGroups();
        qInfo() << "reading drives section of ini";
        settings->beginGroup("drives");
        QStringList childKeys = settings->childKeys();
        for ( const auto& i : childKeys  )
            {
                qInfo() << i << settings->value(i).toString();
                drive_to_slot_map[i]=settings->value(i).toInt();
            }
        qInfo() << drive_to_slot_map;
    } else
    {
        QString errmsg = QString("ERROR unable to open %1").arg(iniFileName);
        QMessageBox msgBox;
        msgBox.setText(errmsg);
        msgBox.exec();
        QApplication::quit();
    }
    qInfo() << "listing drives->slots";
    QMap<QString, int>::iterator it;
    for (it = drive_to_slot_map.begin(); it != drive_to_slot_map.end(); it++)
        qInfo() << it.key() << it.value();

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
    QStringList disks;
    QMap<QString, int>::iterator it;
    for (it = drive_to_slot_map.begin(); it != drive_to_slot_map.end(); it++)
    {
        QString ns1 = "/dev/" + it.key() + "n1";
        QString ns2 = "/dev/" + it.key() + "n2";
        if ( QFileInfo::exists("/dev/" + it.key()) ) disks.append(it.key());
        bool disk_present = QFileInfo::exists("/dev/" + it.key());
        bool ns1_present = QFileInfo::exists(ns1);
        bool ns2_present = QFileInfo::exists(ns2);
        qInfo() << "checking for " << it.key() << it.value() << disk_present << ns1 << ns1_present << ns2 << ns2_present;
    }

}

void MainWindow::setup_display()
{

    disks_present = find_disks();
    running = false;
    setup_CPU_selector();
    QPixmap *green_disk = new QPixmap(":/new/mainimages/hdd-green.png");
    //create list of chassis slots
    QMap<QString,int>::iterator dit;
    //for (int i = 1; i < num_chassis_slots + 1; i++)
    for (dit = drive_to_slot_map.begin(); dit != drive_to_slot_map.end(); dit++)
    {
        //qInfo() << "initializing chassis slot " << i;
        HDD * thisDisk = new HDD();
        Chassis_slot * thisSlot = new Chassis_slot;
        thisDisk->name = dit.key();
        thisDisk->present = QFileInfo::exists("/dev/" + thisDisk->name);
        thisDisk->namespaces[1].name = "/dev/" + dit.key() + "n1";
        thisDisk->namespaces[1].present = QFileInfo::exists(thisDisk->namespaces[1].name);
        thisDisk->namespaces[2].name = "/dev/" + dit.key() + "n2";
        thisDisk->namespaces[2].present = QFileInfo::exists(thisDisk->namespaces[2].name);
        thisDisk->slot_no = dit.value();
        int i = dit.value();
        switch (i){
        case 1:
            thisDisk->my_slot = ui->groupBox_d1;
            thisDisk->my_view[0] = ui->graphicsView_01;
            thisDisk->my_grid_layout = ui->gridLayout_d01;
            break;
        case 2:
            thisDisk->my_slot = ui->groupBox_d2;
            thisDisk->my_view[0] = ui->graphicsView_02;
            thisDisk->my_grid_layout = ui->gridLayout_d02;
            break;
        case 3:
            thisDisk->my_slot = ui->groupBox_d3;
            thisDisk->my_view[0] = ui->graphicsView_03u;
            thisDisk->my_view[1] = ui->graphicsView_03l;
            thisDisk->my_grid_layout = ui->gridLayout_d03;
            break;
        case 4:
            thisDisk->my_slot = ui->groupBox_d4;
            thisDisk->my_view[0] = ui->graphicsView_04;
            thisDisk->my_grid_layout = ui->gridLayout_d04;
            break;
        case 5:
            thisDisk->my_slot = ui->groupBox_d5;
            thisDisk->my_view[0] = ui->graphicsView_05;
            thisDisk->my_grid_layout = ui->gridLayout_d05;
            break;
        case 6:
            thisDisk->my_slot = ui->groupBox_d6;
            thisDisk->my_view[0] = ui->graphicsView_06u;
            thisDisk->my_view[1] = ui->graphicsView_06l;
            thisDisk->my_grid_layout = ui->gridLayout_d06;
            break;
        case 7:
            thisDisk->my_slot = ui->groupBox_d7;
            thisDisk->my_view[0] = ui->graphicsView_07;
            thisDisk->my_grid_layout = ui->gridLayout_d07;
            break;
        case 8:
            thisDisk->my_slot = ui->groupBox_d8;
            thisDisk->my_view[0] = ui->graphicsView_08;
            thisDisk->my_grid_layout = ui->gridLayout_d08;
            break;
        case 9:
            thisDisk->my_slot = ui->groupBox_d9;
            thisDisk->my_view[0] = ui->graphicsView_09u;
            thisDisk->my_view[1] = ui->graphicsView_09l;
            thisDisk->my_grid_layout = ui->gridLayout_d09;
            break;
        case 10:
            thisDisk->my_slot = ui->groupBox_d10;
            thisDisk->my_view[0] = ui->graphicsView_10;
            thisDisk->my_grid_layout = ui->gridLayout_d10;
            break;
        case 11:
            thisDisk->my_slot = ui->groupBox_d11;
            thisDisk->my_view[0] = ui->graphicsView_11;
            thisDisk->my_grid_layout = ui->gridLayout_d11;
            break;
        case 12:
            thisDisk->my_slot = ui->groupBox_d12;
            thisDisk->my_view[0] = ui->graphicsView_12u;
            thisDisk->my_view[1] = ui->graphicsView_12l;
            thisDisk->my_grid_layout = ui->gridLayout_d12;
            break;
        default:
            qInfo() << "Error in disk - UI mapping!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
            break;
        }

        thisDisk->my_view[0]->setMinimumHeight(qd_chart_maxY);
        thisDisk->my_view[0]->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
        thisDisk->my_scene[0] = new QGraphicsScene();
        thisDisk->my_view[0]->setScene(thisDisk->my_scene[0]);
        thisDisk->my_slot->setTitle(thisDisk->name);
        thisDisk->ndx = i;
        thisDisk->num_queues = num_cpus + 1;
        QVBoxLayout *stats = new QVBoxLayout();
        thisDisk->bw_label = new QLabel("mbps");
        thisDisk->iops_label = new QLabel("iops");
        QLabel *bw = new QLabel("BW");
        QLabel *iops = new QLabel("IOPS");
        stats->addWidget(bw);
        stats->addWidget(thisDisk->bw_label);
        stats->addWidget(iops);
        stats->addWidget(thisDisk->iops_label);
        //thisDisk->my_grid_layout->addLayout(stats,1,3);
        if (thisDisk->slot_no == 3 )
        {
            QLabel *bw = new QLabel("BW");
            QLabel *iops = new QLabel("IOPS");
            QVBoxLayout *stats = new QVBoxLayout();
            //thisDisk->my_grid_layout->addLayout(stats,2,3);
        }
        qreal w = thisDisk->my_view[0]->width();
        qreal h = thisDisk->my_view[0]->height();
        qInfo() << "disk:" << i << "view heigth:width" << h << w;
        qreal bar_width = w / (thisDisk->num_queues + 2);
        int bar_margin = 3;
        for (int i = 0; i < thisDisk->num_queues; i++)
        {
            QGraphicsRectItem * r = new QGraphicsRectItem(i*bar_width + bar_margin,0,bar_width,h);
            r->setBrush(QBrush(Qt::red));
            //qInfo() << "bar: X:" << i*bar_width << "width: " << bar_width << "heigth: " <<  h;
            thisDisk->my_scene[0]->addItem(r);
            thisDisk->qBars.push_back(r);
        }
        thisDisk->lastQD = new std::vector<int>;
        for (int j = 0; j < thisDisk->num_queues; j++) {
            // init last qd to 0
            thisDisk->lastQD->insert(thisDisk->lastQD->end(),0);
        }
//        QVBoxLayout* icons = new QVBoxLayout();
//        QLabel * icon = new QLabel();
//        icon->setPixmap(*green_disk);
//        icons->addWidget(icon);
//        icons->addWidget(ndx);
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

        formlayout_TGT_int->addRow(cb_label,thisDisk->selector_checkbox);
        thisDisk->my_grid_layout->addLayout(formlayout_TGT_int,0,0);
//        icons->addLayout(formlayout_TGT_int);
//        thisDisk->hddLayout->addLayout(icons);
//        QVBoxLayout * queues = new QVBoxLayout();
//        thisDisk->hddLayout->addLayout(queues);
//        QLabel *ql = new QLabel("queues");
//        ql->setAlignment(Qt::AlignCenter);
//        queues->addWidget(ql);
//        QFont f( "Arial", 10);
//        int c = 0;
//        for (QProgressBar* qpb : thisDisk->pBars){
//            QHBoxLayout *h = new QHBoxLayout();
//            queues->addLayout(h);
//            QLabel * qn = new QLabel(QString::number(c));
//            qn->setFont(f);
//            h->addWidget(qn);
//            h->addWidget(qpb);
//            c++;
//        }
//        thisDisk->statsLayout = new QVBoxLayout();
//        thisDisk->bw_label = new QLabel("mbps");
//        thisDisk->iops_label = new QLabel("iops");
//        thisDisk->hddLayout->addLayout(thisDisk->statsLayout);
//        thisDisk->statsLayout->addWidget(new QLabel("KBPS"));
//        thisDisk->statsLayout->addWidget(thisDisk->bw_label);
//        thisDisk->statsLayout->addWidget(new QLabel("IOPS"));
//        thisDisk->statsLayout->addWidget(thisDisk->iops_label);
        qInfo() << "tracking disk " << thisDisk->name;
        //disks.insert(disks.end(),thisDisk);
        my_disks[thisDisk->name] = thisDisk;
    }
    qInfo() << disks;

    ui->plainTextEdit->setStyleSheet("QPlainTextEdit {background-color: black; color: red;}");
    ui->action_Start->setEnabled(false);
    ui->StopWork->setEnabled(false);
    qInfo() << "done setup_display()";
    qInfo() << "stat: Initialized";
}

void MainWindow::exercise_qd_display()
{
     QMap<int,int> dummyqd;
     for (HDD *h : disks) {
        qInfo() << "adding disk " << h->name << "to layout";
        for (int qd = 200; qd > 0; qd-=10)
        {
            qInfo() << qd;
            for (int i = 0; i < num_cpus+1; i++)
            {
                dummyqd[i] = qd;
            }
            update_qgraph(h,dummyqd);
            QThread::msleep(1);
        }
    }
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

void MainWindow::handleUpdateIOPS(const DeviceCounters iops_map, const DeviceCounters bw_map )
{
    if ((!update_ok) || (done))
    {
        qInfo() << "received IOPS update with display update disabled";
        return;
    }
    qInfo() << "main thread received IOPs/ BW data" << iops_map << bw_map;

    for (QString d :iops_map.keys())
    {
        int iops = 0;
        for (int ns :iops_map[d].keys())
        {
            iops += iops_map[d][ns];
        }
        my_disks[d]->iops_label->setNum(iops);
    }

    for (QString d :bw_map.keys())
    {
        int iops = 0;
        for (int ns :bw_map[d].keys())
        {
            iops += bw_map[d][ns];
        }
        my_disks[d]->bw_label->setNum(iops);
    }
}

void MainWindow::update_qgraph(HDD* disk, const QMap<int,int> qds)
{
    qInfo() << "update_qgraph received: " << disk->name << qds;
    if (disk->my_slot == nullptr)
    {
        qInfo() << "skipping graph update for missing disk";
        return;
    }

    QList<int> keys = qds.keys();
    for (int i = 0; i < num_cpus+1; i++)
    {
        QRectF r = disk->qBars[i]->rect();
        if (keys.contains(i))
        {
            if (qds[i] > full_scale_qd) r.setTop(0);
            else r.setTop(qd_chart_maxY * (1-qds[i]/full_scale_qd));
            disk->qBars[i]->setRect(r);
        }
        else
        {
            r.setTop(qd_chart_maxY);
            disk->qBars[i]->setRect(r);
        }
    }
}

void MainWindow::handleResults(const Queues results)
{
    if (!update_ok)
    {
        qInfo() << "received QD update with display update disabled";
        for(auto e : disks)
        {
          //TODO clear the q bars
        }
        return;
    }
    qInfo() << "handleResults" << results;
    QStringList disks = results.keys();
    qInfo() << "disks: " << disks;
    const QStringList &localDisks = disks;
    for (QString d : localDisks)
    {
     update_qgraph(my_disks[d],results[d][1]);
     qInfo() << "QD data recd:" << results[d][1] ;
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
    done=false;
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

    workerThread->start();
    ui->statusbar->showMessage("Thread started",1000);
    ui->SetWork->setEnabled(true);
    qInfo() << "done start_system()";
}

void MainWindow::on_actionS_top_triggered()
{
    if (!running) return;
    running = false;
    done=true;
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
    done=true;
}

void MainWindow::on_fio_stdout()
{
    qInfo() << "fio:" << fioProcess->readAllStandardOutput();
    qInfo() << "fio:" << fioProcess->readAllStandardError();
}

QStringList MainWindow::get_fio_targets()
{
    QStringList targets;
    QMap<QString,HDD*>::const_iterator it;
    qInfo() << "my_disks" << my_disks;
    for (it = my_disks.cbegin(); it !=my_disks.constEnd(); it++)
        if (it.value()->present)
        {
        qInfo() << "fio adding " << it.value()->name;
        targets.append(it.value()->namespaces[1].name);
        if (it.value()->namespaces[2].present) targets.append(it.value()->namespaces[2].name);
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
    QMap<QString,HDD*>::ConstIterator h;
    for (h = my_disks.cbegin(); h != my_disks.constEnd(); h++)
    {
        for (int i = 1; i < 2; i++)
            if ((h.value()->namespaces[i].present) && (h.value()->selector_checkbox->isChecked()==true ))
        {
            fioParameters.append(QString("--name=%1").arg(h.value()->namespaces[i].name));
            fioParameters.append(QString("--filename=%1").arg(h.value()->namespaces[i].name));
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
    exercise_qd_display();
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
    emit reset_nvmeworker();
    update_ok = false;
    qInfo() << "done on_StopWork_clicked()";
}

void MainWindow::setup_chassis_serialport()
{
    //depreciated
    qInfo() << "called depreciated function setup_chassis_serialport()";
}

void MainWindow::on_action_Serial_Setup_triggered()
{
    setup_chassis_serialport();
    qInfo() << "done on_action_Serial_Setup_triggered()";
}


void MainWindow::on_pushButton_clicked()
{
    QMap<int,int> dummyqd;
    dummyqd_input += 10;
    if (dummyqd_input > 200) dummyqd_input = 0;
    for (int i = 0; i < num_cpus+1; i++)
    {
        dummyqd[i] = dummyqd_input;
    }
    qInfo() << dummyqd;
    for (HDD *h : disks) {
        update_qgraph(h,dummyqd);
    }
}

