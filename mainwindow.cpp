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
    if (thread_needs_killed)
    {
        qInfo() << "manually killing the worker thread";
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
        const QStringList cK = childKeys;
        for ( const auto& i : cK  )
            {
                qInfo() << i << settings->value(i).toString();
                drive_to_slot_map[i]=settings->value(i).toInt();
            }
        qInfo() << drive_to_slot_map;
        settings->endGroup();
        settings->beginGroup("ui");
        qd_chart_maxY = settings->value("qd_chart_maxY").toReal();
        full_scale_qd = settings->value("full_scale_qd").toReal();
        drive_top_row_heigth = settings->value("drive_top_row_heigth").toInt();
        drive_group_box_heigth = settings->value("drive_group_box_heigth").toInt();
        grid_margin = settings->value("grid_margin").toInt();
        grid_row_0_min_h = settings->value("grid_row_0_min_h").toInt();
        drive_graphics_view_width = settings->value("drive_graphics_view_width").toInt();
        DAdrive_group_box_heigth = settings->value("DAdrive_group_box_heigth").toInt();
        drive_graphics_view_heigth = settings->value("drive_graphics_view_heigth").toInt();
        plain_text_edit_heigth = settings->value("plain_text_edit_heigth").toInt();
        settings->endGroup();
        settings->beginGroup("models");
        childKeys = settings->childKeys();
        const QStringList mK = childKeys;
        for ( const QString& j : mK  )
            {
                qInfo() << j << settings->value(j).toString();
                model_map[j] =settings->value(j).toString();
            }
        settings->endGroup();
        qInfo() << "model map" << model_map;

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
            QVBoxLayout *bl = new QVBoxLayout();
            ui->groupBox_CPU->setLayout(bl);
            formlayout_CPU =  new QFormLayout();

            bl->addLayout(formlayout_CPU);
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
            break;
        case 2:
            thisDisk->my_slot = ui->groupBox_d2;
            break;
        case 3:
            thisDisk->my_slot = ui->groupBox_d3;
            //thisDisk->is_DA = true;
            break;
        case 4:
            thisDisk->my_slot = ui->groupBox_d4;
            break;
        case 5:
            thisDisk->my_slot = ui->groupBox_d5;
            break;
        case 6:
            thisDisk->my_slot = ui->groupBox_d6;
            //thisDisk->is_DA = true;
            break;
        case 7:
            thisDisk->my_slot = ui->groupBox_d7;
            break;
        case 8:
            thisDisk->my_slot = ui->groupBox_d8;
            break;
        case 9:
            thisDisk->my_slot = ui->groupBox_d9;
            //thisDisk->is_DA = true;
            break;
        case 10:
            thisDisk->my_slot = ui->groupBox_d10;
            break;
        case 11:
            thisDisk->my_slot = ui->groupBox_d11;
            break;
        case 12:
            thisDisk->my_slot = ui->groupBox_d12;
            //thisDisk->is_DA = true;
            break;
        default:
            qInfo() << "Error in disk - UI mapping!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
            break;
        }
        thisDisk->my_slot->setFixedHeight(drive_group_box_heigth);
        if (thisDisk->is_DA) thisDisk->my_slot->setFixedHeight(DAdrive_group_box_heigth);
        thisDisk->my_slot->setMinimumWidth(drive_group_box_width);
        thisDisk->my_grid_layout = new QGridLayout();
        thisDisk->my_slot->setLayout(thisDisk->my_grid_layout);
        thisDisk->left_col_layout = new QVBoxLayout();
        QLabel * icon = new QLabel();
        icon->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        icon->setMinimumHeight(80);
        icon->setPixmap(*green_disk);
        thisDisk->left_col_layout->addWidget(icon);
        thisDisk->my_grid_layout->addLayout(thisDisk->left_col_layout,1,0);
        for (int i=0;i<1;i++)
        {
            thisDisk->my_view[i]=new QGraphicsView();
            qInfo() << "setting view h & w: " << drive_graphics_view_heigth << drive_graphics_view_width;
            thisDisk->my_view[i]->setFixedHeight(drive_graphics_view_heigth);
            thisDisk->my_view[i]->setFixedWidth(drive_graphics_view_width);
            thisDisk->my_view[i]->setMaximumHeight(drive_graphics_view_heigth);
            thisDisk->my_view[i]->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
            thisDisk->my_view[i]->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
            thisDisk->my_view[i]->setFrameStyle(QFrame::Box);
            thisDisk->my_scene[i] = new QGraphicsScene();
            thisDisk->my_view[i]->setScene(thisDisk->my_scene[i]);
            if ((i > 1) && (thisDisk->is_DA))
            {
                thisDisk->my_grid_layout->addWidget(thisDisk->my_view[i],i+1,1);
                qInfo() << "added namespace" << i;
            }
            else
            {
                if(i==0)thisDisk->my_grid_layout->addWidget(thisDisk->my_view[i],i+1,1);
                qInfo() << "added my_view at " << 1 << i+1;
            }
        }
        thisDisk->my_grid_layout->addItem(new QSpacerItem(15,15),1,2);
        thisDisk->my_grid_layout->addItem(new QSpacerItem(10,10),2,1);
        thisDisk->my_slot->setTitle(thisDisk->name);
        qInfo() << "grid layout children" << thisDisk->my_grid_layout->children();
        qInfo() << "drive group box children" << thisDisk->my_slot->children();
        for (auto c : thisDisk->my_slot->children())
        {
            if (c->isWidgetType())
            {
                qInfo() << "obj name: " << c->objectName() ;
                if (QString("QLabel") == c->metaObject()->className())
                {
                   QLabel *label = thisDisk->my_slot->findChild<QLabel*>(c->objectName());
                   label->setMaximumHeight(drive_top_row_heigth);
                   label->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
                }

            }
        }

        thisDisk->ndx = i;
        thisDisk->num_queues = num_cpus + 1;
        thisDisk->my_grid_layout->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        thisDisk->my_grid_layout->setMargin(grid_margin);
        thisDisk->my_grid_layout->setSpacing(5);
        thisDisk->my_grid_layout->setRowMinimumHeight(0,grid_row_0_min_h);
        thisDisk->selector_checkbox = new QCheckBox();
        thisDisk->selector_checkbox->setMinimumHeight(25);
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
        thisDisk->left_col_layout->addSpacerItem(new QSpacerItem(10,20) );
        thisDisk->left_col_layout->addLayout(formlayout_TGT_int);
//        thisDisk->left_col_layout->addWidget(cb_label);
//        thisDisk->left_col_layout->addWidget(thisDisk->selector_checkbox,Qt::Alignment(Qt::AlignHCenter)|Qt::Alignment(Qt::AlignTop));
        thisDisk->my_grid_layout->addWidget(new QLabel("QUEUES"),0,1,Qt::Alignment(Qt::AlignHCenter));
        thisDisk->my_grid_layout->addWidget(new QLabel(model_map[thisDisk->name]),2,1,Qt::Alignment(Qt::AlignHCenter));
        QVBoxLayout *stats = new QVBoxLayout();
        thisDisk->bw_label = new QLabel("mbps");
        thisDisk->iops_label = new QLabel("iops");
        QLabel *bw = new QLabel("BW kBps");
        QLabel *iops = new QLabel("IOPS");
        stats->addWidget(bw);
        stats->addWidget(thisDisk->bw_label);
        stats->addWidget(iops);
        stats->addWidget(thisDisk->iops_label);
        int r = 1; int c = 3;
        qInfo() << "adding stats at " << r << c;
        thisDisk->my_grid_layout->addLayout(stats,r,c);
//        if (thisDisk->slot_no == 3 )
//        {
//            QLabel *bw = new QLabel("BW");
//            QLabel *iops = new QLabel("IOPS");
//            QVBoxLayout *stats = new QVBoxLayout();
//            //thisDisk->my_grid_layout->addLayout(stats,2,3);
//        }
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

        qInfo() << "tracking disk " << thisDisk->name;
        //disks.insert(disks.end(),thisDisk);
        my_disks[thisDisk->name] = thisDisk;
    }
    qInfo() << disks;

    ui->plainTextEdit->setStyleSheet("QPlainTextEdit {background-color: black; color: red;}");
    ui->plainTextEdit->setMaximumHeight(plain_text_edit_heigth);
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
    qInfo() << "timer event " << event;
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
        if (d.contains("rq")) continue; // not sure why this is happening
        int iops = 0;
        for (int ns :iops_map[d].keys())
        {
            iops += iops_map[d][ns];
        }
        my_disks[d]->iops_label->setNum(iops);
    }

    for (QString d :bw_map.keys())
    {
        int bw = 0;
        for (int ns :bw_map[d].keys())
        {
            bw += bw_map[d][ns];
        }
        int bwkbps = bw * 4096 / 1024;
        my_disks[d]->bw_label->setNum(bw);
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
     if (d.contains("rq")) continue;  // not sure why this is happening
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

QString MainWindow::get_fio_CPUs()
{
    QString cpus = "--cpus_allowed=";
    int c = 0;
    for (QCheckBox* cb : cpu_boxes)
    {
        if (cb->isChecked())
        {
            cpus.append(QString::number(c));
            cpus.append(",");
            qInfo() << "CPU" << c << "running fio";
        } else
        {
            qInfo() << "CPU" << c << "skipped";
        }
        c++;
    }
    return cpus;
}

int MainWindow::start_fio()
{
    int status = 0;
    QStringList targets = get_fio_targets();
    QStringList fioParameters;
    fioParameters.append("--name=global");
    fioParameters.append("--direct=1");
    fioParameters.append("--ioengine=libaio");
    fioParameters.append(get_fio_CPUs());

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




