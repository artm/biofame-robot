#include "RoboShell.h"
#include "ui_RoboShell.h"

#include "Motor.h"
#include "FaceTracker.h"
#include "SoundSystem.h"
#include "QtOpenCVHelpers.h"

#include <videoInput.h>

#include <QtCore>
#include <QPainter>

// ms
#define POLL_PERIOD 50

RoboShell * RoboShell::s_shell = 0;
QtMsgHandler RoboShell::s_oldMsgHandler = 0;

class GrayTable : public QVector<QRgb> {
public:
    GrayTable()
        : QVector<QRgb>(256)
    {
        for(int i=0;i<256;++i) {
            operator[](i) = qRgb(i,i,i);
        }
    }
};

static const GrayTable grayTable;

QImage toGrayScale(const QImage& img)
{
    QImage gray(img.size(), QImage::Format_Indexed8);
    gray.setColorTable(grayTable);
    for(int i=0;i<img.width(); ++i)
        for(int j=0; j<img.height(); ++j)
            gray.setPixel(i,j,qGray(img.pixel(i,j)));
    return gray;
}

void normalizeGrayscale(QImage& img) {
    Q_ASSERT(img.format() == QImage::Format_Indexed8);
    int min, max;
    min = max = img.pixel(0,0);
    for(int i=0;i<img.width(); ++i)
        for(int j=0; j<img.height(); ++j) {
            int v = img.pixel(i,j);
            if (min > v) min = v;
            if (max < v) max = v;
        }
    int d = max - min;
    for(int i=0;i<img.width(); ++i)
        for(int j=0; j<img.height(); ++j) {
            int v = (img.pixel(i,j) - min) * 255 / d;
            img.setPixel( i, j, v );
        }
}

RoboShell::RoboShell(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::RoboShell)
    , m_boardId(-1)
    , m_cams(new videoInput)
    , m_openCam(-1)
    , m_trackingState(FACE_DETECTION)
    , m_sound(new SoundSystem(this))
    , m_displayMode(0)
    , m_logFile( QString("BioFame-%1.log").arg(QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss")) )
{
    m_logFile.open(QFile::WriteOnly);
    if (m_logFile.isOpen()) {
        m_log.setDevice(&m_logFile);
    }

    ui->setupUi(this);
    ui->statusBar->installEventFilter(this);
    ui->log->hide();
    s_shell = this;
    s_oldMsgHandler = qInstallMsgHandler(msgHandler);

    m_sound->start();
    connect(ui->geiger, SIGNAL(valueChanged(int)), m_sound, SLOT(setGeiger(int)));

    m_faceTracker = new FaceTracker(this);
    if (m_faceTracker->usesVerilook()) {
        connect(ui->minIOD, SIGNAL(valueChanged(int)), m_faceTracker, SLOT(setMinIOD(int)));
        connect(ui->maxIOD, SIGNAL(valueChanged(int)), m_faceTracker, SLOT(setMaxIOD(int)));
        connect(ui->confThresh, SIGNAL(valueChanged(double)), m_faceTracker, SLOT(setConfidenceThreshold(double)));
        connect(ui->qualityThresh, SIGNAL(valueChanged(int)), m_faceTracker, SLOT(setQualityThreshold(int)));

    }
    connect(ui->smin, SIGNAL(valueChanged(int)), m_faceTracker, SLOT(setSMin(int)));
    connect(ui->vmin, SIGNAL(valueChanged(int)), m_faceTracker, SLOT(setVMin(int)));
    connect(ui->vmax, SIGNAL(valueChanged(int)), m_faceTracker, SLOT(setVMax(int)));
    connect(ui->retrackThreshold, SIGNAL(valueChanged(int)),
            m_faceTracker, SLOT(setRetrackThreshold(int)));

    connect(ui->openControllerButton, SIGNAL(toggled(bool)),SLOT(toggleOpenMotors(bool)));

    ui->cameraPanel->setMotor( new Motor(m_boardId, CAMERA) );
    ui->armPanel->setMotor( new Motor(m_boardId, ARM) );
    ui->bodyPanel->setMotor( new Motor(m_boardId, BODY) );
    ui->bodyPanel->setDesireControlsVisible(true);
    ui->wheelsPanel->setMotor( new Motor(m_boardId, WHEELS) );

    connect(this,SIGNAL(boardOpened()), ui->cameraPanel, SLOT(onBoardOpened()));
    connect(this,SIGNAL(boardClosing()), ui->cameraPanel, SLOT(onBoardClosing()));
    connect(this,SIGNAL(boardClosed()), ui->cameraPanel, SLOT(onBoardClosed()));

    connect(this,SIGNAL(boardOpened()), ui->armPanel, SLOT(onBoardOpened()));
    connect(this,SIGNAL(boardClosing()), ui->armPanel, SLOT(onBoardClosing()));
    connect(this,SIGNAL(boardClosed()), ui->armPanel, SLOT(onBoardClosed()));

    connect(this,SIGNAL(boardOpened()), ui->bodyPanel, SLOT(onBoardOpened()));
    connect(this,SIGNAL(boardClosing()), ui->bodyPanel, SLOT(onBoardClosing()));
    connect(this,SIGNAL(boardClosed()), ui->bodyPanel, SLOT(onBoardClosed()));

    connect(this,SIGNAL(boardOpened()), ui->wheelsPanel, SLOT(onBoardOpened()));
    connect(this,SIGNAL(boardClosing()), ui->wheelsPanel, SLOT(onBoardClosing()));
    connect(this,SIGNAL(boardClosed()), ui->wheelsPanel, SLOT(onBoardClosed()));

    connect(ui->layerSelector, SIGNAL(activated(int)), SLOT(setDisplayMode(int)));

    connect(&m_pollTimer, SIGNAL(timeout()), SLOT(motorsTask()));

    connect(this, SIGNAL(faceDetected(QPointF)), m_sound, SLOT(click()));

    buildStateMachine();

    // the last thing to do: open the board and be ready
    ui->openControllerButton->setChecked(true);

    // setup video input
    connect(&m_videoTimer, SIGNAL(timeout()), SLOT(videoTask()));
    connect(&m_videoTimer, SIGNAL(timeout()), SLOT(updateTimeStamp()));
    connect(ui->camSelector, SIGNAL(activated(int)), SLOT(openCamera()));
    connect(ui->resolution, SIGNAL(currentIndexChanged(int)), SLOT(openCamera()));
    connect(ui->camSettings, SIGNAL(clicked()), SLOT(openCamSettings()));

    int nCams = m_cams->listDevices();
    ui->camSelector->setEnabled(nCams);
    ui->camSettings->setEnabled(nCams);
    ui->resolution->setEnabled(nCams);
    ui->camSelector->clear();
    if (nCams) {
        for(int i=0; i<nCams; ++i) {
            ui->camSelector->addItem( m_cams->getDeviceName(i) );
            openCamera();
        }
    } else {
        ui->camSelector->addItem("no video inputs present");
    }

    ui->camForce->setSymmetric(true);
    connect(ui->cameraPanel, SIGNAL(forceFeedback(double)), ui->camForce, SLOT(setValue(double)));
    ui->bodyForce->setSymmetric(true);
    connect(ui->bodyPanel, SIGNAL(forceFeedback(double)), ui->bodyForce, SLOT(setValue(double)));
    ui->armForce->setSymmetric(true);
    connect(ui->armPanel, SIGNAL(forceFeedback(double)), ui->armForce, SLOT(setValue(double)));
    ui->wheelsForce->setSymmetric(true);
    connect(ui->wheelsPanel, SIGNAL(forceFeedback(double)), ui->wheelsForce, SLOT(setValue(double)));

    connect(ui->cameraPanel, SIGNAL(angleChanged(double)), ui->camAngle, SLOT(setValue(double)));
    connect(ui->bodyPanel, SIGNAL(angleChanged(double)), ui->bodyAngle, SLOT(setValue(double)));

    loadSettings();
}

RoboShell::~RoboShell()
{
    m_sound->quit();

    saveSettings();

    m_cams->stopDevice(0);
    delete m_cams;
    if (m_boardId >= 0)
        closeMotors();
    if (m_faceTracker)
        delete m_faceTracker;

    delete ui;
}

void RoboShell::buildStateMachine()
{
    m_machine = new QStateMachine(this);
    m_machine->setGlobalRestorePolicy(QStateMachine::RestoreProperties);

    QState * idle = new QState(m_machine);
    m_machine->setInitialState(idle);
    connect(idle, SIGNAL(entered()), SLOT(stopAllAxes()));

    QState * busy = new QState(m_machine);
    busy->addTransition(ui->panic, SIGNAL(clicked()), idle);

    QState * calib = new QState(QState::ParallelStates, busy);
    idle->addTransition(ui->calibrate, SIGNAL(clicked()), calib);
    ui->cameraPanel->setupCircleCalibState(new QState(calib));
    ui->bodyPanel->setupCircleCalibState(new QState(calib));
    calib->addTransition(calib, SIGNAL(finished()), idle);
    connect(calib, SIGNAL(finished()), SLOT(saveSettings()));
    ui->cameraPanel->setMachine(m_machine);
    ui->cameraPanel->setCircleReset(true);
    ui->bodyPanel->setMachine(m_machine);
    ui->bodyPanel->setCircleReset(true);

    QState * seek = new QState(QState::ParallelStates, busy);
    idle->addTransition(ui->seek, SIGNAL(clicked()), seek);
    ui->cameraPanel->setupSeekState(new QState(seek));
    ui->bodyPanel->setupSeekState(new QState(seek));
    ui->wheelsPanel->setupSeekState(new QState(seek));
    seek->addTransition(seek, SIGNAL(finished()), idle);

    // -----------------------------------------------------------
    // NEW SEEK BEHAVIOR
    QState * newSeek, * search, * track, * refind, * gotcha, * roam;
    addNamedState("seek", newSeek = new QState(busy));
    idle->addTransition(ui->newSeek, SIGNAL(clicked()), newSeek);
    newSeek->addTransition(newSeek, SIGNAL(finished()), idle);

    addNamedState("search", search = new QState(newSeek));
    addNamedState("track", track = new QState(newSeek));
    addNamedState("refind", refind = new QState(newSeek));
    addNamedState("gotcha", gotcha = new QState(newSeek));
    addNamedState("roam", roam = new QState(newSeek));
    newSeek->setInitialState(search);

    // create / configure timers
    m_faceLostTimer.setInterval(1000);
    m_refindTimer.setInterval(2500);
    m_stareTimer.setInterval(5000);
    m_roamTimer.setInterval(5000);

    // connect timeouts to transitions
    track->addTransition(&m_faceLostTimer, SIGNAL(timeout()), refind);
    refind->addTransition(&m_refindTimer, SIGNAL(timeout()), search);
    gotcha->addTransition(&m_stareTimer, SIGNAL(timeout()), roam);
    roam->addTransition(&m_roamTimer, SIGNAL(timeout()), search);
    // non-timeout transitions
    //search->addTransition(this, SIGNAL(faceDetected(QPointF)), track);
    refind->addTransition(this, SIGNAL(faceDetected(QPointF)), track);
    track->addTransition(this, SIGNAL(gotcha()), gotcha);
    // -----------------------------------------------------------

    QState * init = new QState(QState::ParallelStates, busy);
    idle->addTransition(ui->initialize, SIGNAL(clicked()), init);
    ui->cameraPanel->setupInitCircleState( new QState(init) );
    ui->bodyPanel->setupInitCircleState( new QState(init) );
    init->addTransition(init, SIGNAL(finished()), idle);

    m_machine->start();
}

void RoboShell::closeMotors()
{
    if (m_boardId < 0)
        return;

    emit boardClosing();

    // always save...
    if (P1240MotSavePara(m_boardId, 0xF) != ERROR_SUCCESS) {
        qDebug() << "Coudn't save axes parameters to windows registry";
    }

    if (P1240MotDevClose(m_boardId) == ERROR_SUCCESS) {
        qDebug() << "Successfully closed board";
    }
    m_boardId = -1;

    emit boardClosed();

    m_pollTimer.stop();
}

void RoboShell::openMotors(int id)
{
    Q_ASSERT( id >= 0 && id <= 15 );
    closeMotors();
    if (P1240MotDevOpen(id) != ERROR_SUCCESS) {
        qWarning() << "Motor controller wasn't open";
        return;
    }
    m_boardId = id;
    emit boardOpened();
    m_pollTimer.start(POLL_PERIOD);
}

void RoboShell::toggleOpenMotors(bool on)
{
    if (on)
        openMotors();
    else
        closeMotors();

    ui->openControllerButton->setChecked( m_boardId >= 0 );
}

void RoboShell::motorsTask()
{
    if (m_boardId < 0) return;

    /* poll some registers from 4 axes at once */
    DWORD axisData[4];
    if (P1240MotRdMultiReg(m_boardId, 0xF, Lcnt, axisData, axisData+1, axisData+2, axisData+3)
            == ERROR_SUCCESS) {
        ui->cameraPanel->displayPosition(axisData[CAMERA]);
        ui->armPanel->displayPosition(axisData[ARM]);
        ui->bodyPanel->displayPosition(axisData[BODY]);
        ui->wheelsPanel->displayPosition(axisData[WHEELS]);
    }

    if (P1240MotRdMultiReg(m_boardId, 0xF, CurV, axisData, axisData+1, axisData+2, axisData+3)
            == ERROR_SUCCESS) {
        ui->cameraPanel->displaySpeed(axisData[CAMERA]);
        ui->armPanel->displaySpeed(axisData[ARM]);
        ui->bodyPanel->displaySpeed(axisData[BODY]);
        ui->wheelsPanel->displaySpeed(axisData[WHEELS]);

        ui->camSpeed->setValue( axisData[CAMERA] );
        ui->armSpeed->setValue( axisData[ARM] );
        ui->bodySpeed->setValue( axisData[BODY] );
        ui->wheelsSpeed->setValue( axisData[WHEELS] );
    }

    quint8 event_masks[4];
    if( P1240MotCheckEvent(m_boardId, (DWORD*)event_masks, 0) == ERROR_SUCCESS ) {
        ui->cameraPanel->parseEvents(event_masks[CAMERA]);
        ui->armPanel->parseEvents(event_masks[ARM]);
        ui->bodyPanel->parseEvents(event_masks[BODY]);
        ui->wheelsPanel->parseEvents(event_masks[WHEELS]);
    }

    /* make individual axes poll the rest (e.g. inputs) */
    ui->cameraPanel->poll();
    ui->armPanel->poll();
    ui->bodyPanel->poll();
    ui->wheelsPanel->poll();

    machineTick();
}

void RoboShell::stopAllAxes()
{
    ui->cameraPanel->stop();
    ui->armPanel->stop();
    ui->bodyPanel->stop();
    ui->wheelsPanel->stop();
}

void RoboShell::msgHandler(QtMsgType type, const char *message)
{
    s_shell->log(type, message);
    s_oldMsgHandler(type, message);
}

void RoboShell::log(QtMsgType type, const char *message)
{
    if (!ui->log->isVisible()) {
        // this shouldn't be necessary, but it is...
        ui->statusBar->clearMessage();
        ui->statusBar->showMessage(message);
    }

    QListWidgetItem * item = new QListWidgetItem(message);
    switch(type) {
    case QtWarningMsg:
        item->setForeground(QBrush(QColor(150,120,60)));
        break;
    case QtCriticalMsg:
    case QtFatalMsg:
        item->setForeground(QBrush(QColor(128,0,0)));
    case QtDebugMsg:
    default:
        break;
    }

    ui->log->addItem(item);
    while (ui->log->count() > 50) {
        delete ui->log->takeItem(0);
    }
    ui->log->scrollToBottom();

    if (m_logFile.isOpen()) {
        QString tag;

        switch(type) {
        case QtWarningMsg:
            tag = "W";
            break;
        case QtCriticalMsg:
            tag = "E";
            break;
        case QtFatalMsg:
            tag = "F";
            break;
        case QtDebugMsg:
        default:
            tag = "D";
            break;
        }

        m_log << QString("%1 %2: %3\n")
                 .arg(QTime::currentTime().toString("HH:mm:ss.zzz"))
                 .arg(tag)
                 .arg(message);
        m_log.flush();
    }
}

bool RoboShell::eventFilter(QObject * obj, QEvent * e)
{
    if (obj == (QObject*)ui->statusBar && e->type() == QEvent::MouseButtonRelease) {
        ui->log->setVisible( !ui->log->isVisible() );
        if (ui->log->isVisible())
            ui->statusBar->clearMessage();
        else if (ui->log->count() > 0)
            ui->statusBar->showMessage( ui->log->item( ui->log->count()-1 )->text() );
        return true;
    }
    return false;
}

void RoboShell::videoTask()
{
    if ((m_openCam > -1) && m_cams->isFrameNew(m_openCam)) {
        QImage inputFrame  = QImage(m_cams->getWidth(m_openCam),
                                    m_cams->getHeight(m_openCam),
                                    QImage::Format_RGB888);
        m_cams->getPixels(m_openCam, inputFrame.bits(), true, true);


        QImage deinterlaced = ui->deinterlace->isChecked()
                ? inputFrame.scaled(inputFrame.width(), inputFrame.height()/2)
                : inputFrame;

        QImage display =
                deinterlaced.scaled(320, 320*inputFrame.height()/inputFrame.width(),
                                    Qt::IgnoreAspectRatio, Qt::SmoothTransformation).copy();

        if (m_faceTracker) {
            switch(m_trackingState) {

            case FACE_DETECTION: {
                QImage gray = toGrayScale(deinterlaced);
                if (ui->normalize->isChecked())
                    normalizeGrayscale(gray);

                QList<QRect> faces;
                m_faceTracker->findFaces(gray, faces);
                QPointF vector;
                if (faces.size()>0) {
                    notifyOfFace(faces[0],gray);

                    QPainter painter(&display);

                    double sx = (double) display.width() / gray.width();
                    double sy = (double) display.height() / gray.height();
                    QRectF displayFace(faces[0].x()*sx, faces[0].y()*sy, faces[0].width()*sx, faces[0].height()*sy);
                    painter.setPen( QColor(0,255,0,150) );
                    painter.drawRect( displayFace );

                    if (ui->camShift->isChecked()) {
                        // convert faces[0] to deinterlaced coordinates
                        sx = (double) deinterlaced.width() / gray.width();
                        sy = (double) deinterlaced.height() / gray.height();
                        QRect face(faces[0].x()*sx, faces[0].y()*sy, faces[0].width()*sx, faces[0].height()*sy);
                        m_faceTracker->startTracking(deinterlaced, face);
                        m_trackingState = TRACKING;
                    }
                }

                break;
            }
            case TRACKING:
                if (!m_faceTracker->track(deinterlaced)) {
                    m_trackingState = FACE_DETECTION;
                } else {
                    QRect face = QtCv::cutOut( m_faceTracker->trackWindow(), 0.2, 0.3 );
                    notifyOfFace(face,deinterlaced);
                    QPainter painter(&display);
                    double sx = (double) display.width() / deinterlaced.width();
                    double sy = (double) display.height() / deinterlaced.height();
                    QRectF displayFace(face.x()*sx, face.y()*sy, face.width()*sx, face.height()*sy);

                    if (m_displayMode == 1) {
                        painter.drawImage(QRect(0,0,display.width(),display.height()),
                                          m_faceTracker->probabilityImage(),
                                          QRect(0,0,deinterlaced.width(),deinterlaced.height()));
                    }

                    painter.setPen( QColor(0,255,0,150) );
                    painter.drawRect( displayFace );
                    ui->confidence->setValue( m_faceTracker->trackConfidence() );
                }
                break;
            }
        }

        ui->video->setPixmap( QPixmap::fromImage( display ) );

    }
}

void RoboShell::openCamera()
{
    if (m_openCam > -1)
        m_cams->stopDevice(m_openCam);

    m_openCam = ui->camSelector->currentIndex();

    if (m_openCam > -1) {
        m_cams->setIdealFramerate(m_openCam, 25);
        m_cams->setAutoReconnectOnFreeze(m_openCam,true,7);

        if (ui->resolution->currentIndex()) {
            QStringList strs = ui->resolution->currentText().split('x');
            Q_ASSERT(strs.length() == 2);
            m_cams->setupDevice(m_openCam, strs[0].toInt(), strs[1].toInt());
        } else
            m_cams->setupDevice(m_openCam);

        m_videoTimer.start(40);
    }
}

void RoboShell::openCamSettings()
{
    if (ui->camSelector->isEnabled()) {
        m_cams->showSettingsWindow(ui->camSelector->currentIndex());
    }
}

void RoboShell::loadSettings()
{
    QSettings s;

    s.beginGroup("Camera");
    ui->resolution->setCurrentIndex( s.value("resolution").toInt() );
    ui->normalize->setChecked( s.value("normalize").toBool() );
    ui->deinterlace->setChecked( s.value("deinterlace").toBool() );
    s.endGroup();

    s.beginGroup("Robot Motion");
    ui->cameraPanel->loadSettings(s, "Camera");
    ui->armPanel->loadSettings(s, "Arm");
    ui->bodyPanel->loadSettings(s, "Body");
    ui->wheelsPanel->loadSettings(s, "Wheels");
    s.endGroup();

    s.beginGroup("Face tracker");
    ui->sizeCorrA->setValue( s.value("sizeCorrA", 0.0).toDouble() );
    ui->sizeCorrB->setValue( s.value("sizeCorrB", 2.0).toDouble() );
    ui->minIOD->setValue( s.value("minIOD", 5).toInt() );
    ui->maxIOD->setValue( s.value("maxIOD", 320).toInt() );
    ui->confThresh->setValue( s.value("confThresh", 52.0).toDouble() );
    ui->qualityThresh->setValue( s.value("qualityThresh", 128).toInt() );

    ui->camShift->setChecked( s.value("camShift", false).toBool() );
    ui->smin->setValue( s.value("smin", 30).toInt() );
    ui->vmin->setValue( s.value("vmin", 10).toInt() );
    ui->vmax->setValue( s.value("vmax", 255).toInt() );
    ui->retrackThreshold->setValue( s.value("retrackThreshold", 25).toInt() );

    s.endGroup();
}

void RoboShell::saveSettings()
{
    QSettings s;

    s.beginGroup("Camera");
    s.setValue("resolution", ui->resolution->currentIndex());
    s.setValue("normalize", ui->normalize->isChecked());
    s.setValue("deinterlace", ui->deinterlace->isChecked());
    s.endGroup();

    s.beginGroup("Robot Motion");
    ui->cameraPanel->saveSettings(s, "Camera");
    ui->armPanel->saveSettings(s, "Arm");
    ui->bodyPanel->saveSettings(s, "Body");
    ui->wheelsPanel->saveSettings(s, "Wheels");
    s.endGroup();

    s.beginGroup("Face tracker");
    s.setValue("sizeCorrA", ui->sizeCorrA->value());
    s.setValue("sizeCorrB", ui->sizeCorrB->value());
    s.setValue("minIOD", ui->minIOD->value());
    s.setValue("maxIOD", ui->maxIOD->value());
    s.setValue("confThresh", ui->confThresh->value());
    s.setValue("qualityThresh", ui->qualityThresh->value());

    s.setValue("camShift", ui->camShift->isChecked());
    s.setValue("smin", ui->smin->value());
    s.setValue("vmin", ui->vmin->value());
    s.setValue("vmax", ui->vmax->value());
    s.setValue("retrackThreshold", ui->retrackThreshold->value());
    s.endGroup();

}

void RoboShell::updateTimeStamp()
{
    QTime now = QTime::currentTime();
    ui->timestamp->setText( now.toString("HH:mm:ss.zzz") );
}

void RoboShell::notifyOfFace(const QRect &face, const QImage& where)
{
    double distCorr = (double)face.width() / where.width();
    distCorr =
            ui->sizeCorrA->value() * distCorr * distCorr
            + ui->sizeCorrB->value() * distCorr;

    QPointF vector = face.center();
    vector.setX( 2.0 * (vector.x() / where.width() - 0.5) );
    vector.setY( 2.0 * (vector.y() / where.height() - 0.5) );

    vector *= distCorr;

    emit faceDetected(vector);

}

void RoboShell::addNamedState(const QString &tag, QAbstractState *state)
{
    m_states[tag] = state;
    state->setObjectName(tag);
}

void RoboShell::machineTick()
{
    QSet<QAbstractState*> state = m_machine->configuration();

    if (state.contains(m_states["search"])) {
        searchTick();
    } else if (state.contains(m_states["track"])) {
        trackTick();
    } else if (state.contains(m_states["refind"])) {
        refindTick();
    } else if (state.contains(m_states["gotcha"])) {
    } else if (state.contains(m_states["roam"])) {
    }
}

void RoboShell::searchTick()
{
    Motor * cam = ui->cameraPanel->motor();

    switch (cam->motionState()) {
    case Motor::MotionCont:
        cam->stop();
        break;
    case Motor::MotionStopped:
        ui->cameraPanel->gotoAngle( ui->cameraPanel->estimatedAngle() < 0 ? 45 : -45 );
        break;
    default:
        // nothing to do (cont motion or breaking)
        break;
    }
}

void RoboShell::trackTick()
{
}

void RoboShell::refindTick()
{
}

