#ifndef _CONFIG_H_
#define _CONFIG_H_
#include <string>


    class Config
    {
        public:
        static Config* getInstance();
        void LoadConfig();
        bool parseCommandLine(int argc, char**argv);

        std::string USERNAME;
        std::string PASSWORD;
        std::string APPKEY;
        std::string MOUNTPOINT;
		int fuseindex;
        private:
        Config();
        std::string configFile;

        void check_variable(int &,std::string value,std::string name);
        void check_variable(std::string &,std::string value,std::string name);
    };


#endif
