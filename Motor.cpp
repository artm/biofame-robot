#include "Motor.h"
#include <QtDebug>
#include <QMap>

#include <boost/assign/list_of.hpp>
using namespace boost::assign;

#define NOT_OPEN_RETURN(res) if (m_boardId < 0) { qWarning() << "No open board"; return res; }

#define CHECK_RESULT_RES( expr, res ) do { \
        DWORD _err_code_ = expr; \
        if (_err_code_ != ERROR_SUCCESS) { \
            if (ADS1240_ERROR_MESSAGES.count(_err_code_) == 0) \
                qCritical("Error calling " #expr ": 0x%04x", (unsigned int)_err_code_); \
            else \
                qCritical("Error calling " #expr ": %s", ADS1240_ERROR_MESSAGES[_err_code_].c_str()); \
            return res; \
        } \
    } while(false)

#define CHECK_RESULT(expr) CHECK_RESULT_RES(expr,)

static std::map<long, std::string> ADS1240_ERROR_MESSAGES = map_list_of
    (ERROR_SUCCESS,"Function call success")
    (BoardNumErr,"The assigned Board number error")
    (CreateKernelDriverFail,"System return error when open kernel driver")
    (CallKernelDriverFail,"System return error when call kernel driver")
    (RegistryOpenFail,"Open Registry file error")
    (RegistryReadFail,"Read Registry file error")
    (AxisNumErr,"The assigned axis number error")
    (UnderRGErr,"RG value is under valid range")
    (OverRGErr,"RG value is over valid range")
    (UnderSVErr,"SV value is under valid range")
    (OverSVErr,"SV value is over valid range")
    (OverMDVErr,"MDV value is over valid range")
    (UnderDVErr,"DV value is under valid range")
    (OverDVErr,"DV value is over valid range")
    (UnderACErr,"AC value is under valid range")
    (OverACErr,"AC value is over valid range")
    (UnderAKErr,"AK value is under valid range")
    (OverAKErr,"AK value is over valid range")
    (OverPLmtErr,"PLmt value is over valid range")
    (OverNLmtErr,"NLmt value is over valid range")
    (MaxMoveDistErr,"Moving distance is over single maximum distance")
    (AxisDrvBusy,"One of the assigned axis is in busy state")
    (RegUnDefine,"The assigned Register have no defined")
    (ParaValueErr,"One of the assigned parameter value is invalid")
    (ParaValueOverRange,"One of the assigned parameter value is over valid value")
    (ParaValueUnderRange,"One of the assigned parameter value is under valid value")
    (AxisHomeBusy,"One of the assigned axis is in Home process")
    (AxisExtBusy,"One of the assigned axis is in External driving mode")
    (RegistryWriteFail,"Write Registry file error")
    (ParaValueOverErr,"One of the assigned parameter value is over valid value")
    (ParaValueUnderErr,"One of the assigned parameter value is under valid value")
    (OverDCErr,"DC value is over valid range")
    (UnderDCErr,"DC value is under valid range")
    (UnderMDVErr,"MDV value is under valid range")
    (RegistryCreateFail,"Create Registry file fail")
    (CreateThreadErr,"Create internal Thread error")
    (HomeSwStop,"Return Home process be stopped by P1240MotStop function")
    (ChangeSpeedErr,"The speed parameter is invalid")
    (DOPortAsDriverStatus,"Output DO when DO type is assigned to indicator function")
    (ChangePositionErr,"The all axes are stopped")
    (OpenEventFail,"System return error when create IRQ event")
    (DeviceCloseErr,"System return error when in P1240MotDevClose process")
    (HomeEMGStop,"Return home process be stopped by hardware emergency stop input")
    (HomeLMTPStop,"Return home process be stopped by hardware positive limited switch")
    (HomeLMTNStop,"Return home process be stopped by hardware negative limited switch")
    (HomeALARMStop,"Return home process be stopped by software limited switch")
    (AllocateBufferFail,"System return error when buffer allocate")
    (BufferReAllocate,"The assigned buffer have been allocated and try to reallocate again")
    (FreeBufferFail,"The handle of assigned Buffer is NULL or has been freed before")
    (FirstPointNumberFail,"The first data hasn't been set and try to set other number data")
    (PointNumExceedAllocatedSize,"Current buffer is full")
    (BufferNoneAllocate,"The assigned Buffer number hasn't created")
    (SequenceNumberErr,"The point number is not continue number of previous setting data")
    (PathTypeErr,"Path type is not IPO_L2, IPO_L3, IPO_CW or IPO_CCW.")
    (PathTypeMixErr,"Data buffer have mixed IPO_L2 and IPO_L3 or IPO_CW/IPO_CCW and IPO_L3 path type")
    (BufferDataNotEnough,"Buffer contain only one data")
    (FunctionNotSupport,"Only PCI-1240U supports this function");


Motor::Motor(int& boardId, int axisNum, QObject *parent)
    : QObject(parent)
    , m_boardId(boardId)
    , m_axisBit((1 << axisNum)) // double parens for Qt Creator's identation sake
    , m_ui(dynamic_cast<AxisControlPanel*>(parent))
{
}

void Motor::output(quint8 mask)
{
    NOT_OPEN_RETURN();

    // double negation makes sure TRUE is just one bit
    mask &= 0xF; // leave only lowest 4 bits corresponding to outputs 4-7
    CHECK_RESULT( P1240MotDO( m_boardId, m_axisBit, mask) );
}

void Motor::cmove(bool cw)
{
    NOT_OPEN_RETURN();
    // cw is 0 at axis bit, ccw is 1 there
    CHECK_RESULT( P1240MotCmove( m_boardId, m_axisBit, cw ? 0 : m_axisBit) );
}

void Motor::stop()
{
    NOT_OPEN_RETURN();
    // second axis bit means "slow down stop this axis"
    CHECK_RESULT( P1240MotStop( m_boardId, m_axisBit, m_axisBit) );
}

int Motor::getReg(int reg)
{
    NOT_OPEN_RETURN(0);
    DWORD result;
    CHECK_RESULT_RES( P1240MotRdReg( m_boardId, m_axisBit, reg, &result), 0 );
    return (int)result;
}

void Motor::setAxisPara(bool sCurve,
                        int initSpeed, int driveSpeed, int maxDriveSpeed,
                        int acceleration, int accelerationRate)
{
    NOT_OPEN_RETURN();
    CHECK_RESULT( P1240MotAxisParaSet(m_boardId,
                                      m_axisBit,
                                      sCurve ? m_axisBit : 0,
                                      initSpeed, driveSpeed, maxDriveSpeed,
                                      acceleration, accelerationRate ) );
}
