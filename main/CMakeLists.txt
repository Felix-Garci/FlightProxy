idf_component_register(SRCS "main.cpp"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES AppLogic 
                                  Core 
                                  Channel 
                                  PlatformESP32 
                                  Connectivity
								  nvs_flash
								  esp_event
						)
