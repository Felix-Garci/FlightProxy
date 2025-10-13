#include "IbusTransmitter.hpp"
// placeholder

void fcbridge::ibus::IbusTransmitter::setChannelsSource(rc::RcChannels *src)
{
    rc_ = src;
}

void fcbridge::ibus::IbusTransmitter::setStateModel(state::StateModel *state)
{
    state_ = state;
}

void fcbridge::ibus::IbusTransmitter::setUartPort(UartPort *port)
{
    uart_ = port;
}

bool fcbridge::ibus::IbusTransmitter::start()
{
    if (!rc_ || !uart_ || !state_)
        return false;
}

void fcbridge::ibus::IbusTransmitter::stop()
{
}

void fcbridge::ibus::IbusTransmitter::runLoop()
{
}
