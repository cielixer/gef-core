// test_main.cpp - Basic tests
#include <iostream>
#include <gef/PluginLoader.h>
#include <gef/Registry.h>
#include <gef/Context.h>

int main() {
    gef::PluginLoader loader;
    gef::Registry registry;
    gef::Context ctx;
    
    std::cout << "Basic component instantiation test passed!" << std::endl;
    return 0;
}
