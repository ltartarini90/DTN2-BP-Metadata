/* 
 * File:   SortBundleCommand.h
 * Author: luca
 *
 * Created on 9 gennaio 2013, 17.28
 */

#ifndef SORTBUNDLECOMMAND_H
#define	SORTBUNDLECOMMAND_H

#include <oasys/tclcmd/TclCommand.h>
#include <iostream>
#include <fstream>
#include <cmath>
#include <string>
#include <map>

using namespace std;

namespace dtn {

class SortBundleCommand : public oasys::TclCommand {
public:
    SortBundleCommand();

    virtual int exec(int argc, const char** argv, Tcl_Interp* interp);
     
private:
    /* 
     * private function to initialize the priority table from 
     * a specified file name
     */
    int initialize_priority_table(const char *file_name);
        
};

}
#endif	/* SORTBUNDLECOMMAND_H */

