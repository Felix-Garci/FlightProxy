#pragma once

#include "FlightProxy/AppLogic/Command/ICommand.h"
#include <memory>

namespace FlightProxy
{
    namespace AppLogic
    {
        namespace Command
        {
            class CommandManager : public std::enable_shared_from_this<CommandManager>
            {
            public:
                CommandManager();
                ~CommandManager();
                void start();

            private:
            };
        }
    }
}