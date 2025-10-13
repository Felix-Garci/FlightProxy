#include "MspPassthroughProxy.hpp"

fcbridge::msp::MspPassthroughProxy::MspPassthroughProxy(MspSessionManager *session)
{
    session_ = session;
}

bool fcbridge::msp::MspPassthroughProxy::sendToFc(const MspFrame &req)
{
    fcbridge::msp::encode(req, rxBuf_);
    if (write_)
    {
        return write_(rxBuf_);
    }
    return false;
}

void fcbridge::msp::MspPassthroughProxy::onUartBytes(std::span<const uint8_t> bytes)
{
    MspFrame frame;
    fcbridge::msp::decode(bytes, frame);
    notifySession(frame);
}

void fcbridge::msp::MspPassthroughProxy::setUartWriter(UartWrite w)
{
    write_ = w;
}

void fcbridge::msp::MspPassthroughProxy::notifySession(const MspFrame &resp)
{
    if (session_)
    {
        session_->onFcResponse(resp);
    }
}
