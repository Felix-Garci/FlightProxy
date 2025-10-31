#include "FlightProxy/Core/Protocol/MspProtocol.h"
#include "FlightProxy/Channel/UartTransportManager.h"
#include "FlightProxy/Mocks/MockTransport.h"

#include "FlightProxy/Core/Utils/Logger.h" // El singleton Logger de Core
#include "FlightProxy/Mocks/HostLogger.h"  // La implementación de Host (PC)

#include <unity.h>

static FlightProxy::Mocks::HostLogger g_host_logger;

void test_UartTransportManager()
{
    FP_LOG_I("test_UartTransportManager", "Ejecutando test...");

    FlightProxy::Channel::UartTransportManagerT<FlightProxy::Core::MspPacket> uartManager{};
    FlightProxy::Core::Channel::IChannelT<FlightProxy::Core::MspPacket> *Channel = nullptr;

    uartManager.onNewChannel = [&](FlightProxy::Core::Channel::IChannelT<FlightProxy::Core::MspPacket> *channel)
    {
        Channel = channel;
    };

    uartManager.start(
        []() -> FlightProxy::Core::Protocol::IDecoderT<FlightProxy::Core::MspPacket> *
        {
            return new FlightProxy::Core::Protocol::MspDecoder();
        },
        []() -> FlightProxy::Core::Protocol::IEncoderT<FlightProxy::Core::MspPacket> *
        {
            return new FlightProxy::Core::Protocol::MspEncoder();
        },
        []() -> FlightProxy::Core::Transport::ITransport *
        {
            return new FlightProxy::Mocks::MockTransport();
        });

    // Channel->open();
    TEST_ASSERT_NOT_NULL(Channel);
}

void setUp(void)
{
    FP_LOG_D("setUp", "Iniciando nuevo caso de test...");
}

void tearDown(void)
{
    FP_LOG_D("tearDown", "Caso de test finalizado.");
}

int main(void)
{
    FlightProxy::Core::Utils::Logger::setInstance(g_host_logger);

    FP_LOG_I("main", "Iniciando Pruebas Unitarias...");

    UNITY_BEGIN();
    RUN_TEST(test_UartTransportManager);
    return UNITY_END();
}

// En ESP-IDF, se necesita app_main
// (Este fichero solo se usará en 'native', pero es bueno tenerlo)
extern "C" void app_main(void)
{
    main();
}