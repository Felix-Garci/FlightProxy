#include "LocalCtrlHandler.hpp"
#include "utils/Log.hpp"
#include "rc/RcChannels.hpp"
// placeholder

void fcbridge::msp::LocalCtrlHandler::setRc(rc::RcChannels *rc)
{
    rc_ = rc;
}

void fcbridge::msp::LocalCtrlHandler::setState(state::StateModel *st)
{
    st_ = st;
}

fcbridge::msp::MspFrame fcbridge::msp::LocalCtrlHandler::handle(const MspFrame &ctrlReq)
{
    if (rc_ == nullptr || st_ == nullptr)
    {
        return makeError(ctrlReq.cmd, 1); // Error: no inicializado
    }

    if (!ctrlReq.isRequest)
    {
        return makeError(ctrlReq.cmd, 2); // Error: no es request
    }

    switch (ctrlReq.cmd)
    {
    case MspCodes::MSP_SET_RC_DEFAULT:
        if (!ctrlReq.payload.empty())
        {
            return makeError(ctrlReq.cmd, 3); // Error: tamaño inválido
        }
        else
        {
            utils::Log::info("Setting RC defaults via MSP");
            rc_->applyDefaults();

            return makeAck(ctrlReq.cmd);
        }

    case MspCodes::MSP_SET_RC_VALUE:
        if (ctrlReq.payload.size() != sizeof(rc::RcSample))
        {
            return makeError(ctrlReq.cmd, 3); // Error: tamaño inválido
        }
        else
        {
            utils::Log::info("Setting RC values via MSP");
            rc::RcSample sample;
            for (size_t i = 0; i < rc::RC_MAX_CHANNELS; ++i)
            {
                size_t off = i * 2;
                uint16_t v = static_cast<uint16_t>(ctrlReq.payload[off]) | (static_cast<uint16_t>(ctrlReq.payload[off + 1]) << 8);
                sample.ch[i] = v;
            }
            setRcValue(sample);
            return makeAck(ctrlReq.cmd);
        }

    case MspCodes::MSP_GET_RC_STATUS:
        if (!ctrlReq.payload.empty())
        {
            return makeError(ctrlReq.cmd, 3); // Error: tamaño inválido
        }
        else
        {
            utils::Log::info("Getting RC status via MSP");
            return getRcStatus();
        }

    case MspCodes::MSP_SET_UDP_ENABLED:
        if (ctrlReq.payload.size() != 1)
        {
            return makeError(ctrlReq.cmd, 3); // Error: tamaño inválido
        }
        else
        {
            utils::Log::info("Setting UDP enabled via MSP");
            bool on = ctrlReq.payload[0] != 0;
            setUdpEnabled(on);
            return makeAck(ctrlReq.cmd);
        }

    case MspCodes::MSP_SET_TIMEOUT_MS:
        if (ctrlReq.payload.size() != 2)
        {
            return makeError(ctrlReq.cmd, 3); // Error: tamaño inválido
        }
        else
        {
            utils::Log::info("Setting UDP timeout ms via MSP");
            uint16_t ms = static_cast<uint16_t>(ctrlReq.payload[0]) | (static_cast<uint16_t>(ctrlReq.payload[1]) << 8);
            setTimeoutMs(ms);
            return makeAck(ctrlReq.cmd);
        }
    case MspCodes::MSP_SET_IBUS_PERIOD_MS:
        if (ctrlReq.payload.size() != 2)
        {
            return makeError(ctrlReq.cmd, 3); // Error: tamaño inválido
        }
        else
        {
            utils::Log::info("Setting iBUS period ms via MSP");
            uint16_t ms = static_cast<uint16_t>(ctrlReq.payload[0]) | (static_cast<uint16_t>(ctrlReq.payload[1]) << 8);
            setIbusPeriodMs(ms);
            return makeAck(ctrlReq.cmd);
        }
    default:
        return makeError(ctrlReq.cmd, 4); // Error: comando desconocido
    }
}

void fcbridge::msp::LocalCtrlHandler::setRcDefault(const rc::RcSample &s)
{
    rc_->applyDefaults();
}

void fcbridge::msp::LocalCtrlHandler::setRcValue(const rc::RcSample &s)
{
    rc_->setFromLocal(s);
}

fcbridge::msp::MspFrame fcbridge::msp::LocalCtrlHandler::getRcStatus()
{
    fcbridge::rc::RcSnapshot snap = rc_->snapshot();
    utils::Log::info("RC status: flag= %u age=%u ms", snap.current.flags, snap.age);
    return makeStatusPayload(snap);
}

void fcbridge::msp::LocalCtrlHandler::setUdpEnabled(bool on)
{
    st_->setUdpEnabled(on);
}

void fcbridge::msp::LocalCtrlHandler::setTimeoutMs(uint16_t ms)
{
    st_->setUdpTimeoutMs(ms);
}

void fcbridge::msp::LocalCtrlHandler::setIbusPeriodMs(uint16_t ms)
{
    st_->setIbusPeriodMs(ms);
}

fcbridge::msp::MspFrame fcbridge::msp::LocalCtrlHandler::makeAck(uint16_t cmd)
{
    MspFrame frame;
    frame.cmd = cmd;
    frame.isRequest = false;
    frame.result = 0;
    return frame;
}

fcbridge::msp::MspFrame fcbridge::msp::LocalCtrlHandler::makeError(uint16_t cmd, uint16_t code)
{
    MspFrame frame;
    frame.cmd = cmd;
    frame.isRequest = false;
    frame.result = code;
    return frame;
}

fcbridge::msp::MspFrame fcbridge::msp::LocalCtrlHandler::makeStatusPayload(const rc::RcSnapshot &snap)
{
    MspFrame frame;
    frame.isRequest = false;
    frame.result = 0;

    const size_t N = rc::RC_MAX_CHANNELS;
    const size_t offCurrent = 0;      // 0 .. 2N-1
    const size_t offDefaults = 2 * N; // 2N .. 4N-1
    const size_t offAge = 4 * N;      // 4N

    const size_t payloadSize = 4 * N + 4;
    frame.payload.resize(payloadSize);

    // current[0..N-1] (uint16 LE)
    for (size_t i = 0; i < N; ++i)
    {
        const uint16_t v = snap.current.ch[i];
        frame.payload[offCurrent + i * 2 + 0] = static_cast<uint8_t>(v & 0xFF);
        frame.payload[offCurrent + i * 2 + 1] = static_cast<uint8_t>(v >> 8);
    }

    // defaults[0..N-1] (uint16 LE)
    for (size_t i = 0; i < N; ++i)
    {
        const uint16_t v = snap.defaults.ch[i];
        frame.payload[offDefaults + i * 2 + 0] = static_cast<uint8_t>(v & 0xFF);
        frame.payload[offDefaults + i * 2 + 1] = static_cast<uint8_t>(v >> 8);
    }
    // age (uint32 LE)
    const uint32_t age = snap.age;
    frame.payload[offAge + 0] = static_cast<uint8_t>((age >> 0) & 0xFF);
    frame.payload[offAge + 1] = static_cast<uint8_t>((age >> 8) & 0xFF);
    frame.payload[offAge + 2] = static_cast<uint8_t>((age >> 16) & 0xFF);
    frame.payload[offAge + 3] = static_cast<uint8_t>((age >> 24) & 0xFF);

    return frame;
}
