//
// Created by ender on 2026/4/20.
//

#include <iostream>
#include <string>
#include "MapDrawer.h"
#include "plan.h"
using namespace cns;

const std::string testmap1=
        "0123456789\n"
        "=        =\n"
        "=        =\n"
        "= @      =\n"
        "0123456789";


const char* testmap2=
        "============================================================================\n"
        "=                                                                          =\n"
        "=                                                                          =\n"
        "=                                          ==========                      =\n"
        "=                                                                          =\n"
        "=                                    ===                                   =\n"
        "=                                                                          =\n"
        "=                              ===                                         =\n"
        "=                                                                          =\n"
        "=                       ===                                                =\n"
        "=                                                                          =\n"
        "=                 ===                                                      =\n"
        "=                                                                          =\n"
        "=         =======                                                          =\n"
        "=  @                                                                       =\n"
        "============================================================================\n"
        "============================================================================\n"
        "============================================================================";


MapDrawer::MapDrawer() {
    currentmap=testmap2;
}

void MapDrawer::draw() {
    std::cout<<"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";
    std::cout<<currentmap;

}