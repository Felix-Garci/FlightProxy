#pragma once

// --- Includes de Implementaciones Concretas ---
#include "FlightProxy/Channel/ChannelT.h"
#include "FlightProxy/Channel/ChannelServer.h"
#include "FlightProxy/Channel/ChannelPersistent.h"

#include "FlightProxy/Transport/ListenerTCP.h"
#include "FlightProxy/Transport/SimpleTCP.h"
#include "FlightProxy/Transport/SimpleUart.h"

#include <memory>
#include <functional>

namespace FlightProxy
{
    class AppFactory
    {
    public:
    };
}