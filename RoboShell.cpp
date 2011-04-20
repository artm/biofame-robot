#include "RoboShell.h"
#include "ui_RoboShell.h"

#include "Motor.h"
#include "videoInput.h"

#include <QtCore>

#include "FaceTracker.h"

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
{
    ui->setupUi(this);
    ui->statusBar->installEventFilter(this);
    ui->log->hide();
    s_shell = this;
    s_oldMsgHandler = qInstallMsgHandler(msgHandler);

    m_faceTracker = FaceTracker::make();

    connect(ui->openControllerButton, SIGNAL(toggled(bool)),SLOT(toggleOpenMotors(bool)));

    ui->cameraPanel->setMotor( new Motor(m_boardId, CAMERA) );
    ui->armPanel->setMotor( new Motor(m_boardId, ARM) );
    ui->bodyPanel->setMotor( new Motor(m_boardId, BODY) );
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

    connect(this, SIGNAL(faceDetected(QPointF)), ui->cameraPanel, SLOT(trackX(QPointF)));
    connect(ui->cameraPanel, SIGNAL(positionChanged(int)),
            ui->bodyPanel, SLOT(trackAxis(int)));
    connect(ui->bodyPanel, SIGNAL(angleChanged(double)),
            ui->wheelsPanel, SLOT(trackAxisDirection(double)));

    connect(&m_pollTimer, SIGNAL(timeout()), SLOT(motorsTask()));

    buildStateMachine();

    ui->cameraPanel->setTrackCoeff(-500);

    // the last thing to do: open the board and be ready
    ui->openControllerButton->setChecked(true);

    // setup video input
    connect(&m_videoTimer, SIGNAL(timeout()), SLOT(videoTask()));
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

    loadSettings();

    ui->forceXindi->setSymmetric(true);
    ui->forceYindi->setSymmetric(true);
}

RoboShell::~RoboShell()
{
    saveSettings();

    delete ui;
    m_cams->stopDevice(0);
    delete m_cams;
    if (m_boardId >= 0)
        closeMotors();
    if (m_faceTracker)
        delete m_faceTracker;
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

}

void RoboShell::stopAllAxes()
{
    ui->cameraPanel->stop();
    ui->armPanel->stop();
    ui->bodyPanel->stop();
    ui->wheelsPanel->stop();
}

void RoboShell::buildStateMachine()
{
    m_automaton = new QStateMachine(this);

    QState * idle = new QState(m_automaton);
    m_automaton->setInitialState(idle);
    connect(idle, SIGNAL(entered()), SLOT(stopAllAxes()));

    QState * busy = new QState(m_automaton);
    busy->addTransition(ui->panic, SIGNAL(clicked()), idle);

    QState * calib = new QState(busy);
    idle->addTransition(ui->calibrate, SIGNAL(clicked()), calib);
    calib->setChildMode(QState::ParallelStates);
    ui->cameraPanel->setupCircleCalibState(new QState(calib));
    ui->bodyPanel->setupCircleCalibState(new QState(calib));
    calib->addTransition(calib, SIGNAL(finished()), idle);

    QState * seek = new QState(busy);
    idle->addTransition(ui->seek, SIGNAL(clicked()), seek);
    seek->setChildMode(QState::ParallelStates);
    ui->cameraPanel->setupSeekState(new QState(seek));
    ui->bodyPanel->setupSeekState(new QState(seek));
    ui->wheelsPanel->setupSeekState(new QState(seek));
    seek->addTransition(seek, SIGNAL(finished()), idle);

    QState * init = new QState(busy);
    idle->addTransition(ui->initialize, SIGNAL(clicked()), init);
    init->setChildMode(QState::ParallelStates);
    ui->cameraPanel->setupInitCircleState( new QState(init) );
    ui->bodyPanel->setupInitCircleState( new QState(init) );
    init->addTransition(init, SIGNAL(finished()), idle);

    m_automaton->start();
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
    ui->log->scrollToBottom();
    while (ui->log->count() > 500) {
        delete ui->log->takeItem(0);
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
    QPointF vector;
    if ((m_openCam > -1) && m_cams->isFrameNew(m_openCam)) {
        m_cams->getPixels(m_openCam, m_frame.bits(), true, true);

        QImage deinterlaced = ui->deinterlace->isChecked()
                ? m_frame.scaled(m_frame.size()/2)
                : m_frame;

        QImage gray = toGrayScale(deinterlaced);
        if (ui->normalize->isChecked())
            normalizeGrayscale(gray);

        ui->video->setPixmap(
                    QPixmap::fromImage(
                        gray.scaledToWidth(320)));

        if (m_faceTracker) {
            QList<QRect> faces;
            m_faceTracker->process(gray, faces);
            if (faces.size()>0) {
                vector = faces[0].center();
                vector.setX( vector.x() / gray.width() - 0.5 );
                vector.setY( vector.y() / gray.height() - 0.5 );
            }
        }

    }
    ui->forceXindi->setValue(vector.x());
    ui->forceYindi->setValue(vector.y());
    emit faceDetected(vector);
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

        m_frame = QImage(m_cams->getWidth(m_openCam),
                         m_cams->getHeight(m_openCam),
                         QImage::Format_RGB888);
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
}
