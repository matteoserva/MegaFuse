#include "Config.h"
#include <iostream>
#include <string.h>
#include <unistd.h>

using namespace std;
Config* Config::getInstance()
{
    static Config config;
    return &config;
}


void Config::check_variable(int &var,std::string value,std::string name)
{
    var=stoi(value);
    printf("caricata la variabile %s con il valore %d\n",name.c_str(),var);

}

void Config::check_variable(std::string &var,std::string value,std::string name)
{
    var = value;
    printf("caricata la variabile %s con il valore %s\n",name.c_str(),value.c_str());


}

bool Config::parseCommandLine(int argc, char**argv)
{
    //true if you must stop
    opterr = 0;
    for (int c; (c = getopt (argc, argv, ":hc:p:")) != -1;)
    {

        switch (c)
        {
        case 'c':
            configFile=optarg;
            cout <<"ss "<<configFile<<endl;
            break;
        case 'h':
            cout<<"-c configfile    for the config file"<<endl;
            cout<<"-h               help "<<endl;
            return true;
        case '?':
            if (isprint (optopt))
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf (stderr,"Unknown option character `\\x%x'.\n",optopt);
            return true;
        case ':':
            fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            return true;
        default:
            return true;
        }
    }

    if(optind < argc)
    {
        for (int index = optind; index < argc; index++)
            printf ("Non-option argument %s\n", argv[index]);
        return true;
    }
    return false;
}

void Config::LoadConfig()
{

    FILE* fp = fopen(configFile.c_str(), "r");
    if(!fp)
    {
        cerr<<"Couldn't open config file: "<<configFile<<endl;
    }
    else
    {
        char linebuf[256];
        char strbuf[32];
        char valbuf[256];
        wchar_t wstr[256];

        fseek(fp, 0, SEEK_END);
        size_t fsize = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        int linenum=0;

        #define CHECK_VARIABLE(VAR) else if(!strcmp(strbuf,#VAR)) check_variable(VAR,valbuf,#VAR)

        for(int linenum=0; ftell(fp) < fsize; linenum++)
        {
            fgets(linebuf, 250, fp);
            int scanfSuccess = sscanf(linebuf, "%s = %[^\n]s", strbuf, valbuf);
            if(scanfSuccess <= 0 || strbuf[0] == '#')
            {
                //ignored line
            }
            else if(scanfSuccess < 2)
            {
                cerr<<"Could not parse line "<<linenum<<": "<<linebuf<<endl;
            }


            CHECK_VARIABLE(USERNAME);
            CHECK_VARIABLE(PASSWORD);
            CHECK_VARIABLE(APPKEY);
            CHECK_VARIABLE(MOUNTPOINT);
            else
                cerr<<"Could not understand the keyword at line"<<linenum<<": "<<strbuf<<endl;
        }
        #undef CHECK_VARIABLE

        fclose(fp);
    }

}

Config::Config():configFile("megafuse.conf")
{

}
