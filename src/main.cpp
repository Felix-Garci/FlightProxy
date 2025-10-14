#include "net/TcpMspEndpoint.hpp"
#include "msp/MspSessionManager.hpp"
#include "msp/MspPassthroughProxy.hpp"
#include "msp/LocalCtrlHandler.hpp"
#include "rc/RcChannels.hpp"
#include "state/StateModel.hpp"
#include "ibus/IbusTransmitter.hpp"
#include "utils/log.hpp"

using namespace fcbridge;

int app_main(void)
{
    // Logs
    utils::Log::init("transponder");

    // Instancias
    rc::RcChannels rc;
    state::StateModel st;
    msp::LocalCtrlHandler ctrl;
    msp::MspSessionManager session;
    msp::MspPassthroughProxy proxy(&session);
    net::TcpMspEndpoint tcp(&session);
    ibus::IbusTransmitter ibus;

    // Wiring
    ctrl.setRc(&rc);
    ctrl.setState(&st);
    session.setCtrlHandler(&ctrl);
    session.setProxy(&proxy);
    session.setTcpEndpoint(&tcp);
    ibus.setChannelsSource(&rc);
    ibus.setStateModel(&st);

    // start() / UART/TCP setLowLevel... aquí

    return 0;
}
