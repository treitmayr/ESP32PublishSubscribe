cmake_minimum_required(VERSION 3.16)

idf_component_register(
    SRCS src/DeferredCallsQueue.cpp
    INCLUDE_DIRS include
    REQUIRES ${requires}
    PRIV_REQUIRES ${priv_requires}
)
