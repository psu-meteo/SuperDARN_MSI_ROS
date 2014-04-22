#include "global_server_variables.h"
#include "site.h"
#include "iniparser.h"

extern dictionary *Site_INI;

int _open_ini_file() {
     char ini_name[80]="";
     char entry_name[80]="";
     int exists_flag;
     int temp_int;
     char *envstr=NULL;
     char site_dir[128]="";

     envstr=getenv("MSI_SITE_DIR");
     if (envstr == NULL) {
       sprintf(site_dir,"%s",SITE_DIR);
     } else {
       sprintf(site_dir,"%s",envstr);
     }


     if(Site_INI!=NULL) {
       iniparser_freedict(Site_INI);
       Site_INI=NULL;
     }
     sprintf(ini_name,"%s/site.ini",site_dir);
     fprintf(stderr, "parsing ini file: %s\n", ini_name);
     Site_INI=iniparser_load(ini_name);
     if (Site_INI==NULL) {
	fprintf(stderr, "cannot parse file: %s\n", ini_name);
	return -1;
     }
     return 0;
}

