#pragma once

namespace FlightProxy
{
    namespace AppLogic
    {
        namespace Command
        {
            class ICommand
            {
            public:
                virtual ~ICommand() = default;
                virtual void execute() = 0;
            };
        }
    }
}