#pragma once

#include <functional>
namespace FlightProxy
{
    namespace AppLogic
    {
        namespace DataNode
        {
            class IDataNodeBase
            {
            public:
                virtual ~IDataNodeBase() = default;
                virtual void transact() = 0;
            };
        } // namespace DataNode
    } // namespace AppLogic
} // namespace FlightProxy
