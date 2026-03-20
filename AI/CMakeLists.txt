add_library(AILib STATIC
    ModelManager/LLMBackend.cpp
    ModelManager/ModelManager.cpp
    Context/ProjectContext.cpp
    Memory/MemoryStore.cpp
    Embeddings/EmbeddingEngine.cpp
    AgentScheduler/AgentScheduler.cpp
    ErrorLearning/ErrorLearning.cpp
    CodeLearning/CodeLearning.cpp
    Prompts/PromptLibrary.cpp
    Sandbox/Sandbox.cpp
    ArchiveLearning/ArchiveLearning.cpp
    AssetLearning/AssetLearning.cpp
    Training/TrainingPipeline.cpp
)
target_include_directories(AILib PUBLIC ${CMAKE_SOURCE_DIR})
target_compile_features(AILib PUBLIC cxx_std_20)
