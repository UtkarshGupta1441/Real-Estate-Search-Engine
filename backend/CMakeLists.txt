cmake_minimum_required(VERSION 3.15)
project(RealEstateBackend CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch Asio
FetchContent_Declare(
  asio
  GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
  GIT_TAG asio-1-28-0
)
FetchContent_Populate(asio)

# Fetch Crow
FetchContent_Declare(
  Crow
  GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
  GIT_TAG v1.2.0
)
FetchContent_Populate(Crow)

add_executable(backend src/main.cpp)

# Since Crow and Asio are header-only, we just include their directories
target_include_directories(backend PRIVATE 
  ${asio_SOURCE_DIR}/asio/include
  ${crow_SOURCE_DIR}/include
)

# Enable ASIO standalone
target_compile_definitions(backend PRIVATE ASIO_STANDALONE)

# We might need threading on some platforms
find_package(Threads REQUIRED)
target_link_libraries(backend PRIVATE Threads::Threads)
