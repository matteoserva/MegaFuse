#include "Config.h"
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <dirent.h>
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
    for (int c; (c = getopt (argc, argv, ":hfc:p:")) != -1;)
    {

        switch (c)
        {
        case 'c':
            configFile=optarg;
            cout <<"ss "<<configFile<<endl;
            break;
        case 'f':
            fuseindex = optind;
            return false;
            break;
        case 'h':
            cout<<"-c configfile    for the config file"<<endl;
			cout<<"-f args          arguments passed to the fuse module"<<endl;
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
char * Config::getString(std::string prompt, bool isPassword)
{
	static char buffer[STRINGSIZE];
	std::cout << prompt;

		struct termios oldt, newt;
		tcgetattr( STDIN_FILENO, &oldt);
		newt = oldt;

		/*setting the approriate bit in the termios struct*/
		if(isPassword)
		newt.c_lflag &= ~(ECHO);          

		/*setting the new bits*/
		tcsetattr( STDIN_FILENO, TCSANOW, &newt);
		unsigned int i = 0;
		int c;
		/*reading the password from the console*/
		while ((c = getchar())!= '\n' && c != EOF && i < STRINGSIZE){
			buffer[i++] = c;
		}
		buffer[i] = '\0';

		/*resetting our old STDIN_FILENO*/ 
		tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
		if(isPassword)
		std::cout <<std::endl;
		return buffer;	
}

static int countEntriesInDir(std::string dirname)
{
        int n=0;
        dirent* d;
        DIR* dir = opendir(dirname.c_str());
        if (dir == NULL) return -1;
        while((d = readdir(dir))!=NULL) n++;
        closedir(dir);
        return n;
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

        fseek(fp, 0, SEEK_END);
        size_t fsize = ftell(fp);
        fseek(fp, 0, SEEK_SET);

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
            CHECK_VARIABLE(CACHEPATH);
            else
                cerr<<"Could not understand the keyword at line"<<linenum<<": "<<strbuf<<endl;
        }
        #undef CHECK_VARIABLE

        fclose(fp);
    }

	if(USERNAME == "")
	    USERNAME = getString("Username (email): ",false);
	if(PASSWORD == "")
	    PASSWORD = getString("Enter your password: ",true);
	while(2 != countEntriesInDir(MOUNTPOINT))
		MOUNTPOINT = getString("Specify a valid mountpoint (an empty directory): ",false);
	while(0 > countEntriesInDir(CACHEPATH))
		CACHEPATH = getString("Specify a valid cache path (eg: /tmp): ",false);
}

Config::Config():APPKEY("MEGASDK"),fuseindex(-1),configFile("megafuse.conf")
{

}
