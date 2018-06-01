#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdbool.h>

bool get_cpuinfo(char* cpuinfo)
{
        FILE *fp;
		char *buff = 0;
   		size_t size = 0;
		bool ret = false;

		if(!cpuinfo)
		{
			printf("Where shall we store cpuinfo?\n");
		}

		fp= fopen("/proc/cpuinfo", "r");
        if(fp<0){
                printf("We can not get cpu info.\n");
                return false;
        }

	   while(getline(&buff, &size, fp) != -1)
	   {
	      if(!strncmp(buff,"model name",10))
	      {
			strncpy(cpuinfo,buff,80);
			ret = true;
			break;
		  }
	   }

		free(buff);
		fclose(fp);

		return ret;
}

int main(int argc, char* argv){
        char str[81];
        memset(str,0,81);

		if(get_cpuinfo(str))
			printf("CPU: %s\n",str);
		else
			printf("CPU: Unkown\n");

        return;
}
