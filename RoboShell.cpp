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

RoboShell::RoboShell(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::RoboShell)
    , m_boardId(-1)
    , m_videoInput(new videoInput)
    , m_faceTracker( FaceTracker::make() )
{
    ui->setupUi(this);
    s_shell = this;
    s_oldMsgHandler = qInstallMsgHandler(msgHandler);

    connect(ui->openControllerButton, SIGNAL(toggled(bool)),SLOT(toggleOpen(bool)));

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

    m_pollTimer = new QTimer(this);
    connect(m_pollTimer, SIGNAL(timeout()), SLOT(poll()));

    buildStateMachine();

    // the last thing to do: open the board and be ready
    ui->openControllerButton->setChecked(true);

    // start capture...
    m_videoInput->listDevices();
    m_videoInput->setupDevice(0);
    m_frame = QImage(m_videoInput->getWidth(0),
                     m_videoInput->getHeight(0),
                     QImage::Format_RGB888);
}

RoboShell::~RoboShell()
{
    delete ui;
    m_videoInput->stopDevice(0);
    delete m_videoInput;
    if (m_boardId >= 0)
        close();
}

void RoboShell::close()
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

    m_pollTimer->stop();
}

void RoboShell::open(int id)
{
    Q_ASSERT( id >= 0 && id <= 15 );
    close();
    if (P1240MotDevOpen(id) != ERROR_SUCCESS) {
        qWarning() << "Motor controller wasn't open";
        return;
    }
    m_boardId = id;
    emit boardOpened();

    m_pollTimer->start(POLL_PERIOD);
}

void RoboShell::toggleOpen(bool on)
{
    if (on)
        open();
    else
        close();

    ui->openControllerButton->setChecked( m_boardId >= 0 );
}

void RoboShell::poll()
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

    /* make individual axes poll the rest (e.g. inputs) */
    ui->cameraPanel->poll();
    ui->armPanel->poll();
    ui->bodyPanel->poll();
    ui->wheelsPanel->poll();

    if (m_videoInput->isFrameNew(0)) {
        m_videoInput->getPixels(0, m_frame.bits(), true, true);

        QImage deinterlaced =
                m_frame.scaled(m_frame.size()/2);
        QImage gray = toGrayScale(deinterlaced);

        ui->video->setPixmap(
                    QPixmap::fromImage(
                        gray.scaledToWidth(320)));
    }
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
    m_automaton->addTransition(ui->calibrate, SIGNAL(clicked()), calib);
    idle->addTransition(ui->calibrate, SIGNAL(clicked()), calib);
    calib->setChildMode(QState::ParallelStates);
    ui->cameraPanel->insertCircleCalibState(calib);
    ui->bodyPanel->insertCircleCalibState(calib);
    calib->addTransition(calib, SIGNAL(finished()), idle);

    m_automaton->start();
}

void RoboShell::msgHandler(QtMsgType type, const char *message)
{
    s_shell->ui->statusBar->showMessage(message);
    s_oldMsgHandler(type, message);
}
