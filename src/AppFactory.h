#pragma once

#include "FlightProxy/AppLogic/MainController.h" // Desde AppLogic
#include <memory>                                // Para std::unique_ptr

// Esta clase es la "Fábrica"
class AppFactory
{
public:
    // El trabajo de esta función es construir la app
    // y devolver la clase "raíz" de la lógica.
    // std::unique_ptr<FlightProxy::AppLogic::MainController> createApplication();
};