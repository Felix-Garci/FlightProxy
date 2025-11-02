#include "FlightProxy/Core/Protocol/MspProtocol.h"
#include "FlightProxy/Channel/SimpleTransportManagerT.h"
#include "FlightProxy/Mocks/MockTransport.h"

#include "FlightProxy/Mocks/SimpleTcpMock.h"

#include "FlightProxy/Core/Utils/Logger.h" // El singleton Logger de Core
#include "FlightProxy/Mocks/HostLogger.h"  // La implementación de Host (PC)

#include <unity.h>
#include <thread>

static FlightProxy::Mocks::HostLogger g_host_logger;

void test_SimpleTransportManager()
{
    FP_LOG_I("test_SimpleTransportManager", "Ejecutando test...");

    FlightProxy::Channel::SimpleTransportManagerT<FlightProxy::Core::MspPacket> uartManager{};
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

    TEST_ASSERT_NOT_NULL(Channel);
    Channel->open(); // channel open llama a transport open

    FlightProxy::Core::MspPacket p = {
        .direction = '>',
        .command = 0,
        .payload = {0x01, 0x02, 0x03}};

    bool finished = false;

    Channel->onPacket = [&](const FlightProxy::Core::MspPacket &packet)
    {
        FP_LOG_I("Main", "Received MSP Packet: Command=%u, Payload Size=%zu", packet.command, packet.payload.size());
        TEST_ASSERT_EQUAL(p.command, packet.command);
        TEST_ASSERT_EQUAL_HEX8_ARRAY(p.payload.data(), packet.payload.data(), 3);
        finished = true;
    };

    Channel->sendPacket(p);

    while (!finished)
        ;
}

void test_SimpleTransportManager_TCP()
{

    FP_LOG_I("test_SimpleTransportManager TCP", "Ejecutando test...");

    FlightProxy::Channel::SimpleTransportManagerT<FlightProxy::Core::MspPacket> uartManager{};
    FlightProxy::Core::Channel::IChannelT<FlightProxy::Core::MspPacket> *Channel = nullptr;

    uartManager.onNewChannel = [&](FlightProxy::Core::Channel::IChannelT<FlightProxy::Core::MspPacket> *channel)
    {
        FP_LOG_D("test_SimpleTransportManager TCP", "ASignando Channel");
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
            return new FlightProxy::Mocks::SimpleTcpMock("127.0.0.1", 12345);
        });

    TEST_ASSERT_NOT_NULL(Channel);

    FlightProxy::Core::MspPacket p = {
        .direction = '>',
        .command = 0,
        .payload = {0x01, 0x02, 0x03}};

    bool finished = false;

    Channel->onPacket = [&](const FlightProxy::Core::MspPacket &packet)
    {
        FP_LOG_I("Main", "Received MSP Packet: Command=%u, Payload Size=%zu", packet.command, packet.payload.size());
        TEST_ASSERT_EQUAL(p.command, packet.command);
        TEST_ASSERT_EQUAL_HEX8_ARRAY(p.payload.data(), packet.payload.data(), 3);
        finished = true;
        FP_LOG_I("Main", "Test terminado");
    };

    bool connected = false;
    Channel->onOpen = [&]()
    {
        connected = true;
    };

    Channel->open();

    while (!connected)
        ;

    Channel->sendPacket(p);

    while (!finished)
        ;
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
    RUN_TEST(test_SimpleTransportManager);
    RUN_TEST(test_SimpleTransportManager_TCP);
    return UNITY_END();
}

// En ESP-IDF, se necesita app_main
// (Este fichero solo se usará en 'native', pero es bueno tenerlo)
extern "C" void app_main(void)
{
    main();
}