/* 
 * File:   SortBundleCommand.cc
 * Author: luca
 * 
 * Created on 9 gennaio 2013, 17.28
 */

#ifdef HAVE_CONFIG_H
#  include <dtn-config.h>
#endif

#include <oasys/util/StringBuffer.h>
#include <oasys/serialize/XMLSerialize.h>

#include "SortBundleCommand.h"
#include "CompletionNotifier.h"

#include "contacts/Link.h"
#include "contacts/ContactManager.h"

#include "bundling/BundleEvent.h"
#include "bundling/BundleDaemon.h"
#include "routing/TableBasedRouter.h"

namespace dtn {

// global variable to store the file name of the priority table 
char static priority_file[256]; 
    
SortBundleCommand::SortBundleCommand():TclCommand("sortbundle")
{
    add_to_help("order <type>", "set the bundle order of the deferred list "
            "(default FIFO).\n"
            "valid options:\n"
            "fifo - first in first out\n"
            "lifo - last in first out\n"
            "metadata_ascending - metadata ascending alphabetical\n"
            "metadata_descending - metadata descending alphabetical\n"
            "priority_table - sort the forwarding bundle "
            "compared to a keyword and a priority specified in a file");
    
    add_to_help("init_table <filename>", "load from file the policy to "
            "set the priority of the bundles in the deffered list");
    
    add_to_help("keyword <keyword>", "show the priority of the specified "
            "keyword (need the loading of the priority file before issue "
            "this subcommand)");
    
    add_to_help("display <argument>", "displays on the console the "
            "specified argument.\n"
            "valid options:\n"
            "priority_table - displays the priority table\n"
            "order - displays the order of the bundle to forward");
    
    add_to_help("refresh", "refresh the priority table from the previous "
            "file specified");
    
    add_to_help("priority_table <filename>", "set the bundle order as priority_table "
            "and load from file the policy to set the priority of the bundles "
            "in the deffered list");
}

int
SortBundleCommand::exec(int argc, const char** argv, Tcl_Interp* interp)
{
    (void)interp;
    
    if (argc < 2) 
    {
        resultf("need a sortbundle subcommand");
        return TCL_ERROR;
    }

    const char* cmd = argv[1];
    
    if (strcmp(cmd, "order") == 0) 
    {
        // sortbundle order <type>
        if (argc < 3) 
        {
            wrong_num_args(argc, argv, 2, 3, INT_MAX);
            return TCL_ERROR;
        }
        
        const char* sort_order = argv[2];
        
        if(strcmp(sort_order,"fifo") == 0)
           BundleRouter::config_.bundle_fwd_order_ = 
                   BundleList::SORT_FIFO;
        else if(strcmp(sort_order,"lifo") == 0)
           BundleRouter::config_.bundle_fwd_order_ = 
                   BundleList::SORT_LIFO;
        else if(strcmp(sort_order,"metadata_ascending") == 0)
            BundleRouter::config_.bundle_fwd_order_ = 
                    BundleList::SORT_METADATA_ASCENDING_ALPHABETICAL;
        else if(strcmp(sort_order,"metadata_descending") == 0)
           BundleRouter::config_.bundle_fwd_order_ = 
                   BundleList::SORT_METADATA_DESCENDING_ALPHABETICAL;
        else if(strcmp(sort_order,"priority_table") == 0)
        {
            BundleRouter::config_.bundle_fwd_order_ = 
                    BundleList::SORT_PRIORITY_TABLE;
            if(BundleRouter::config_.prioritymap_.empty())
                append_resultf("priority table not initialized\n");
        }
        else 
        {
            resultf("invalid argument '%s'", argv[2]);
            return TCL_ERROR;
        }
        append_resultf("order: %s", sort_order);
    }
    else if (strcmp(cmd, "init_table") == 0) 
    {
        // sortbundle init_table <file_name>
        if (argc < 3) 
        {            
            wrong_num_args(argc, argv, 2, 3, INT_MAX);
            return TCL_ERROR;
        }
        strcpy(priority_file, "");
        const char* file_name = argv[2];
        strcpy(priority_file, file_name);
        int result = initialize_priority_table(file_name);
        if(result == TCL_ERROR)
            return result;
    }
    else if (strcmp(cmd, "keyword") == 0) 
    {
        // sortbundle keyword <keyword>
        if (argc < 3) 
        {            
            wrong_num_args(argc, argv, 2, 3, INT_MAX);
            return TCL_ERROR;
        }
        
        if(BundleRouter::config_.prioritymap_.empty())
        {
            resultf("table not initialized...");
            return TCL_ERROR;
        } 
        const char* keyword = argv[2];
        bool found = false;
        std::map<std::string, int>::iterator iter; 
        for(iter=BundleRouter::config_.prioritymap_.begin(); 
                iter != BundleRouter::config_.prioritymap_.end(); ++iter)
        {
            if(iter->first.compare(keyword) == 0)
                found = true;
        }
        if(found)
        {
            int priority;
            append_resultf("keyword: %s", keyword);
            append_resultf("\nKeyword\t\t\tPriority");
            priority = BundleRouter::config_.prioritymap_.find(keyword)->second;
            append_resultf("\n%s\t\t\t%d", keyword, priority); 
        }
        else
            resultf("keyword not found");
    }
    else if (strcmp(cmd, "display") == 0) 
    {
        // sortbundle display <argument>
        if (argc < 3) 
        {
            wrong_num_args(argc, argv, 2, 3, INT_MAX);
            return TCL_ERROR;
        }
        
        const char* argument = argv[2];
        
        if(strcmp(argument,"priority_table") == 0)
        {
            if(BundleRouter::config_.prioritymap_.empty())
            {
                resultf("table not initialized...");
                return TCL_ERROR;
            } 
            append_resultf("Keyword\t\t\tPriority\n");
            std::map<int,string> mirror_map;
            // Mappa ordinata secondo la priorità
            std::map<std::string, int>::iterator map_iter;
            for(map_iter = BundleRouter::config_.prioritymap_.begin();
                    map_iter != BundleRouter::config_.prioritymap_.end(); 
                    map_iter++)
            {            
                append_resultf("%s\t\t\t%d", map_iter->first.c_str(), 
                        map_iter->second);
                if(map_iter->second < 0)
                    append_resultf("\t-> suborder: LIFO\n");
                else
                    append_resultf("\t-> suborder: FIFO\n");
                mirror_map.insert(std::pair<int,string>(abs(map_iter->second), 
                        map_iter->first));
            }
            append_resultf("\n\nPriority table sorted by priority:\n"
                    "Priority\t\t\tKeyword\n");
            std::map<int,string>::iterator mirror_map_iter;
            for(mirror_map_iter = (mirror_map.end().operator --());	
                    mirror_map_iter != mirror_map.begin().operator --(); 
                    mirror_map_iter--)
            {   
                append_resultf("%d\t\t\t%s", mirror_map_iter->first, 
                        mirror_map_iter->second.c_str());
                for(map_iter = BundleRouter::config_.prioritymap_.begin();
                        map_iter != BundleRouter::config_.prioritymap_.end(); 
                        map_iter++)
                {
                    if(map_iter->first.compare(mirror_map_iter->second) == 0)
                    {
                        if(map_iter->second < 0)
                            append_resultf("\t-> suborder: LIFO\n");
                        else
                            append_resultf("\t-> suborder: FIFO\n");
                    }
                }
            }
        }
        else if(strcmp(argument, "order") == 0)
        {
            if(BundleRouter::config_.bundle_fwd_order_ == BundleList::SORT_FIFO)
                resultf("order: fifo");
            else if(BundleRouter::config_.bundle_fwd_order_ == BundleList::SORT_LIFO)
                resultf("order: lifo");
            else if(BundleRouter::config_.bundle_fwd_order_ == BundleList::SORT_METADATA_ASCENDING_ALPHABETICAL)
                resultf("order: metadata_ascending");
            else if(BundleRouter::config_.bundle_fwd_order_ == BundleList::SORT_METADATA_DESCENDING_ALPHABETICAL)
                resultf("order: metadata_descending");
            else if(BundleRouter::config_.bundle_fwd_order_ == BundleList::SORT_PRIORITY_TABLE)
                resultf("order: priority_table");       
        }
    }
    else if (strcmp(cmd, "refresh") == 0) 
    {
        // sortbundle refresh
        if (argc < 2) 
        {            
            wrong_num_args(argc, argv, 2, 2, INT_MAX);
            return TCL_ERROR;
        }
        
        if(strcmp(priority_file,"") == 0)
        {
            resultf("priority table never initialized");
            return TCL_ERROR;
        }
        
        append_resultf("refresh from file: %s", priority_file);
        int result = initialize_priority_table(priority_file);
        
        if(result == TCL_ERROR)
            return result;
    }
    else if (strcmp(cmd, "priority_table") == 0) 
    {
        // sortbundle priority_table <filename>
        if (argc < 3) 
        {
            wrong_num_args(argc, argv, 2, 3, INT_MAX);
            return TCL_ERROR;
        }
           
        BundleRouter::config_.bundle_fwd_order_ = BundleList::SORT_PRIORITY_TABLE;
        append_resultf("order: priority_table\n");

        
        strcpy(priority_file, "");
        const char* file_name = argv[2];
        strcpy(priority_file, file_name);
        int result = initialize_priority_table(file_name);
        if(result == TCL_ERROR)
            return result;
    }
    else 
    {
        resultf("unimplemented sortbundle subcommand %s", cmd);
        return TCL_ERROR;
    }
    
    return TCL_OK;
}

int 
SortBundleCommand::initialize_priority_table(const char *file_name)
{
    ifstream f(file_name);
    string line;
    char keyword[256] = "";
    char p[256] = "";
    int priority;

    BundleRouter::config_.prioritymap_.clear();

    if(!f.is_open())
    {
        resultf("file: %s does not exist", file_name);
        return TCL_ERROR;
    }

    append_resultf("Keyword\t\t\tPriority");

    while(getline(f, line).good())
    {
        istringstream s( line );

        s.get(keyword, 256, ' ');
        s.get(p, 256, '\n');
        priority = atoi(p);
        BundleRouter::config_.prioritymap_.insert(std::pair<string,int>(keyword,priority));
        append_resultf("\n%s\t\t\t%d", keyword, priority);

        strcpy(keyword, "");
        strcpy(p, "");
    }
    append_resultf("\npriority table initialized...");
    f.close();
    
    return TCL_OK;
}

} // namespace dtn
