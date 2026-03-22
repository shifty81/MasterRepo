add_library(KnowledgeIngestion STATIC
    IngestionManager.cpp
)

target_include_directories(KnowledgeIngestion PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
