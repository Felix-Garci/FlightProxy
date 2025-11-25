#pragma once

#include "FlightProxy/AppLogic/DataNode/IDataNodeBase.h"

#include <vector>
#include <memory>

namespace FlightProxy
{
    namespace AppLogic
    {
        namespace DataNode
        {
            class DataNodesManager
            {
            private:
                std::vector<std::shared_ptr<IDataNodeBase>> m_dataNodes;

            public:
                DataNodesManager()
                {
                }

                void addDataNode(std::shared_ptr<IDataNodeBase> dataNode, uint16_t samplingPeriodMs)
                {
                    m_dataNodes.push_back(dataNode);
                }
            };
        } // namespace DataNode
    } // namespace AppLogic
} // namespace FlightProxy