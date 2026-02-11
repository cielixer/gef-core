// test_main.cpp - Basic tests
#include <gef/Context.h>
#include <gef/PluginLoader.h>
#include <gef/Registry.h>
#include <iostream>

int main() {
    gef::PluginLoader loader;
    gef::Registry registry;
    gef::Context ctx;

    std::cout << "Basic component instantiation test passed!" << std::endl;
    return 0;
}
