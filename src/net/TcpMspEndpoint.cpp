#include "TcpMspEndpoint.hpp"
// placeholder

fcbridge::net::TcpMspEndpoint::TcpMspEndpoint(msp::MspSessionManager *session)
{
    session_ = session;
}
bool fcbridge::net::TcpMspEndpoint::start()
{
    // placeholder
    return true;
}
void fcbridge::net::TcpMspEndpoint::stop()
{
    // placeholder
}
bool fcbridge::net::TcpMspEndpoint::sendToClient(const msp::MspFrame &resp)
{
    fcbridge::msp::encode(resp, rxBuf_);
    sendRaw_(rxBuf_);
    return true;
}

void fcbridge::net::TcpMspEndpoint::onTcpBytes(std::span<const uint8_t> bytes)
{
    fcbridge::msp::MspFrame frame;
    fcbridge::msp::decode(bytes, frame);
    session_->submit(frame);
}
void fcbridge::net::TcpMspEndpoint::setLowLevelSender(SendFunc f)
{
    sendRaw_ = f;
}

void fcbridge::net::TcpMspEndpoint::handleIncomingFrame(const msp::MspFrame &f)
{
    // placeholder
}