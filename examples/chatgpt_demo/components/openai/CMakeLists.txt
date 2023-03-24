idf_component_register(
    SRCS "OpenAI.c" 
    INCLUDE_DIRS "include"
    REQUIRES tcp_transport esp_http_client
)