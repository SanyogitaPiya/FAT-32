#define _GNU_SOURCE
	
	#include <stdio.h>
	#include <unistd.h>
	#include <sys/wait.h>
	#include <stdlib.h>
	#include <errno.h>
	#include <string.h>
	#include <signal.h>
    #include <stdbool.h>
    #include <stdint.h>
    #include <ctype.h>
	
	#define MAX_NUM_ARGUMENTS 10
	
	#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
	                                // so we need to define what delimits our tokens.
	                                // In this case  white space
	                                // will separate the tokens on our command line
	
	#define MAX_COMMAND_SIZE 255    // The maximum command-line size

    //Function declarations
    bool compare(char* IN, char* input);
    int LBAToOffset(int32_t s);
    int16_t NextLB(uint32_t s);

    //This bool variable lets us know if the file is opened or not
    bool opened_or_not=false;
	
    //The file pointer to open files
    FILE* file_pointer;

    //The following is to hold the address of our root dir
    int dir_root;

    //The following is the starting sector information of fat32 file before reaching the root directory
    uint16_t BPS;
    uint8_t SPC;
    uint16_t ReservedSC;
    uint8_t FatsNum;
    uint32_t Fz32;
    int32_t dir_current;

    //The struct is for the information of all the directories from root directory
    struct __attribute__((__packed__)) DirectoryEntry
    {
        char name_of_dir[11];
        uint8_t attribute_of_dir;
        uint8_t not_in_use1[8];
        uint16_t high_clus;
        uint8_t not_in_use2[4];
        uint16_t low_clus;
        uint32_t size_of_file;

    };

    //The array holds all the directories in the root which is 16
    struct DirectoryEntry DE[16];
	
int main()
{
	
	char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );
	
	while( 1 )
	{
	    // Print out the mfs prompt
	    printf ("mfs> ");
	
	    // Read the command from the commandline.  The
	    // maximum command that will be read is MAX_COMMAND_SIZE
	    // This while command will wait here until the user
	    // inputs something since fgets returns NULL when there
	    // is no input
	    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

        //The following is for when user presses enter it should display msh>>
        bool enter=strcmp(cmd_str,"\n");
        if(enter==0)
        continue;
	
        //********************************************************************************************************
	    /* Parse input */
	    char *t[MAX_NUM_ARGUMENTS];
	
	    int   tc = 0;                                 
	                                                           
	    // Pointer to point to the token
	    // parsed by strsep
	    char *arg_ptr;                                         
	                                                           
	    char *ws  = strdup( cmd_str );                
	
	    // we are going to move the working_str pointer so
	    // keep track of its original value so we can deallocate
	    // the correct amount at the end
	    char *wr = ws;
	
	    // Tokenize the input stringswith whitespace used as the delimiter
	    while ( ( (arg_ptr = strsep(&ws, WHITESPACE ) ) != NULL) && 
	              (tc<MAX_NUM_ARGUMENTS))
	    {
	      t[tc] = strndup( arg_ptr, MAX_COMMAND_SIZE );
	      if( strlen( t[tc] ) == 0 )
	      {
	        t[tc] = NULL;
	      }
	        tc++;
	    }
	
	  //***********************************************************************************************************//
	
        if((strcmp(t[0],"open"))==0) //token[0] holds open, then we will look if another file is already opened or not
        {
            if(opened_or_not==false)  //if no other file is opened then we open the file
            {
                file_pointer=fopen(t[1],"r");

                if(file_pointer==NULL)   //condition if it fails to open file
                {
                    printf("Error: File system image not found.\n");
                    continue;
                }

                printf("File opened successfully.\n"); //otherwise opened successfully

                opened_or_not=true; //we set the boolean to true indicating that file is now opened
            }

            else
            {
                printf("Error: File System image already open.\n");
                continue;
            }

            //Now taking all the information about the initial part of the file system
            fseek(file_pointer, 11, SEEK_SET);
            fread(&BPS, 2, 1, file_pointer);

            fseek(file_pointer, 13, SEEK_SET);
            fread(&SPC, 1, 1, file_pointer);

            fseek(file_pointer, 14, SEEK_SET);
            fread(&ReservedSC, 2, 1, file_pointer);

            fseek(file_pointer, 16, SEEK_SET);
            fread(&FatsNum, 1, 2, file_pointer);

            fseek(file_pointer, 36, SEEK_SET);
            fread(&Fz32, 4, 1, file_pointer);  

            //Now we find the address of the root directory
            dir_root = (ReservedSC * BPS) + (FatsNum * Fz32 * BPS);

            // fseek(fp,0x100400,SEEK_SET);
            fseek(file_pointer,dir_root,SEEK_SET);

            fread(DE,sizeof(struct DirectoryEntry),16,file_pointer);
           
        }

        //Now we move onto the close command
        else if((strcmp(t[0],"close"))==0)
        {
            if(file_pointer!=NULL&&opened_or_not==true)
            {
                fclose(file_pointer);
                file_pointer=NULL;
                opened_or_not=false;
                memset(DE,0,sizeof(DE));
            }

            else
            {
                printf("Error: File System not open.\n");
            }
        }

        //Now the printf instruction to show that file system must be opened first
        else if(file_pointer==NULL&&opened_or_not==false)
        {
            printf("Error: File System must be opened first.\n");
            continue;
        }

        //Now for the info command
        else if((strcmp(t[0],"info"))==0)
        {
			printf("BPB_BytsPerSec: %3x  || %3d", BPS, BPS);
			printf("\nBPB_SecPerClus: %3x  || %3d", SPC, SPC);
			printf("\nBPB_RsvdSecCnt: %3x  || %3d", ReservedSC, ReservedSC);
			printf("\nBPB_NumFATs: %6x  || %3d", FatsNum, FatsNum);
			printf("\nBPB_FATSz32: %6x  || %3d\n", Fz32, Fz32);
        }

        //Now for the stat command
        else if((strcmp(t[0],"stat"))==0)
        {
            if(file_pointer!=NULL&&tc==3)
            {
                int i;
	            int captured = 0;
                char* fn=t[1];
	            for (i = 0; i < 16; i++)
	            {
		            if (DE[i].attribute_of_dir == 0x01 || DE[i].attribute_of_dir == 0x10 || DE[i].attribute_of_dir == 0x20)
		            {
			            char count[111];
			            strcpy(count, fn);

			            char count2[12];
			            memset(&count2, 0, 12);
			            strncpy(count2, DE[i].name_of_dir, 11);

			            if ((strcmp(fn, "."))==0 || (strcmp(fn, ".."))==0)
			            {
				            if (strstr(DE[i].name_of_dir, fn) != NULL)
				            {
					            
					            if (DE[i].low_clus == 0)
					            {
						            DE[i].low_clus = 2;
					            }

					            printf("DIR_Name: %22s\n",count2);
                                printf("DIR_Attr: %13d\n",DE[i].attribute_of_dir);

                                printf("DIR_FirstClusterLow: %d\n",DE[i].low_clus);
                                printf("DIR_FileSize: %11d\n", DE[i].size_of_file);

                                //After printing the stats we set the captured variable to 1
					            captured = 1;
                                //Then we break from the loop
					            break;
				            }
			            }

			            else if (compare(DE[i].name_of_dir, count)==true)
			            {
				            printf("DIR_Name: %22s\n",count2);
                            printf("DIR_Attr: %13d\n",DE[i].attribute_of_dir);
                            printf("DIR_FirstClusterLow: %d\n",DE[i].low_clus);
                            printf("DIR_FileSize: %11d\n", DE[i].size_of_file);
				            captured = 1;
			            }	
		            }

	            }
                //now if the file is not found, then
                if (captured == 0)
	            {
		            printf("Error: File not found.\n");
	            }

	
            }
        }

        //Now for the cd command
        else if((strcmp(t[0],"cd"))==0)
        {
            if(tc!=3)
            {
                printf("Not proper format for cd.\n");
                continue;
            }

            int j;
            char* not_old;
            not_old=strtok(t[1],"/");

            if((strcmp(not_old,"."))==0||((strcmp(not_old,".."))==0))
            {
                for(j=0;j<16;j++)
                {
                    if(strstr(DE[j].name_of_dir,not_old)!=NULL)
                    {
                        if(DE[j].low_clus==0)
                        DE[j].low_clus=2;

                        int L=LBAToOffset(DE[j].low_clus);
                        fseek(file_pointer,L,SEEK_SET);
                        fread(&DE[0],sizeof(struct DirectoryEntry),16,file_pointer);

                        break;
                    }
                }
            }

            else
            {
                char count[111];
                for(j=0;j<16;j++)
                {
                    strcpy(count,not_old);
                    bool A=compare(DE[j].name_of_dir,count);
                    if(A&&DE[j].attribute_of_dir!=0x20)
                    {
                        int L=LBAToOffset(DE[j].low_clus);
                        fseek(file_pointer,L,SEEK_SET);
                        fread(&DE[0],sizeof(struct DirectoryEntry),16,file_pointer);

                        break;
                        
                    }
                }
            }
            while((not_old=strtok(NULL,"/")))
            {
                if((strcmp(not_old,"."))==0||((strcmp(not_old,".."))==0))
                {
                    for(j=0;j<16;j++)
                    {
                        if(strstr(DE[j].name_of_dir,not_old)!=NULL)
                        {
                            if(DE[j].low_clus==0)
                            DE[j].low_clus=2;

                            int L=LBAToOffset(DE[j].low_clus);
                            fseek(file_pointer,L,SEEK_SET);
                            fread(&DE[0],sizeof(struct DirectoryEntry),16,file_pointer);

                            break;
                        }
                    }
                }

                else
                {
                    char count[111];
                    for(j=0;j<16;j++)
                    {
                        strcpy(count,not_old);
                        bool A=compare(DE[j].name_of_dir,count);
                        if(A&&DE[j].attribute_of_dir!=0x20)
                        {
                            int L=LBAToOffset(DE[j].low_clus);
                            fseek(file_pointer,L,SEEK_SET);
                            fread(&DE[0],sizeof(struct DirectoryEntry),16,file_pointer);

                            break;
                        
                        }
                    }
                }
            }

        }

        //Now for the ls command
        else if((strcmp(t[0],"ls"))==0)
        {
            //This is for when the user types only ls
            if (tc == 2)
                {
                    int j;
                    char fn[12];
                    for (j = 0; j < 16; j++)
                    {
                        strncpy(fn, DE[j].name_of_dir, 11);
                        fn[11]='\0';

                        if (fn[0] != 0xffffffe5&&(DE[j].attribute_of_dir == 0x01 || DE[j].attribute_of_dir == 0x10 
                            || DE[j].attribute_of_dir == 0x20))
                        {
                            printf("%s\n", fn );
                        }
                    }
                }

                //Now when the user types ls .
                else if (tc == 3)
                {
                    if ((strcmp(t[0],"ls"))==0&&strcmp(t[1], ".") == 0)
                    {
                        int j;
                        char fn[12];
                        for (j = 0; j < 16; j++)
                        {
                            strncpy(fn, DE[j].name_of_dir, 11);
                            fn[11]='\0';

                            if (fn[0] != 0xffffffe5&&(DE[j].attribute_of_dir == 0x01 || DE[j].attribute_of_dir == 0x10 
                                || DE[j].attribute_of_dir == 0x20))
                            {
                                printf("%s\n", fn );
                            }
                        }
                    }
                

                    //Now when user types ls . .
                    else
                    {
                        int found = 0,i;
                        
                        //This is a temporary struct to hold the data of p or c
                        struct DirectoryEntry TD[16];
                        char * dotdot="..";
                        for (i = 0; i < 16; i++)
                        {
                            if(strncmp(dotdot,t[1],2)==0)
                            {
                                if(strncmp(t[1],DE[i].name_of_dir,2)==0)
                                {
                                    int c = DE[i].low_clus;
                                    if(c == 0)
                                    {
                                        c = 2;
                                    }
                                    //Now calculating the offset by calling LBAToOffset
                                    int o = LBAToOffset(c);
                                    fseek(file_pointer,o,SEEK_SET);
                                    fread(TD, sizeof(struct DirectoryEntry), 16, file_pointer);
                                    found = 1;
                                    break;
                                }
                            }
                        }

                        
                        if(found==0)
                        {
                            printf("Not proper argument for ls\n");
                        }

                        
                        if(found==1)
                        {
                            for (i = 0; i < 16; i++)
                            {
                                char fn[12];
                                strncpy(fn, TD[i].name_of_dir, 11);
                                fn[11]='\0';

                                if (fn[0] != 0xffffffe5 && (TD[i].attribute_of_dir == 0x01 || TD[i].attribute_of_dir == 0x10 
                                || TD[i].attribute_of_dir == 0x20))
                                {
                                    printf("%s\n", fn );
                                }

                               

                            }
                        }
                    }
                }
              
                
             
        }

        else if((strcmp("read",t[0]))==0)
        {
            
            continue;
        }
            
        //Now for get
        else if(strcmp("get",t[0])==0)
        {
            char* fn=t[1];
            int j,counter=0;

            for(j=0;j<16;j++)
            {
                char not_old[111];
                strcpy(not_old,fn);

                bool A=compare(DE[j].name_of_dir,not_old);
                if(A)
                {
                    counter=10;
                    break;
                }
            }

            if(counter==0)
            printf("Error: File not found.\n");

            else
            {
                int c=DE[j].low_clus;
                int o=LBAToOffset(c);

                int s=DE[j].size_of_file;

                fseek(file_pointer,o,SEEK_SET);

                //Now we create the output file
                FILE * output=fopen(fn,"w");

                char veg[512];

                if(s<512)
                {
                    fread(&veg[0],s,1,file_pointer);
                    fwrite(&veg[0],s,1,output);
                }

                else
                {
                    fread(&veg[0],512,1,file_pointer);
                    fwrite(&veg[0],512,1,output);
                    s=s-512;

                    for(s;s>0;s=s-512)
                    {
                        c=NextLB(c);
                        o=LBAToOffset(c);

                        fseek(file_pointer,o,SEEK_SET);
                        fread(&veg[0],512,1,file_pointer);
                        fread(&veg[0],512,1,output);   
                    }
                }

                //Now closing the output file
                fclose(output);
                
            }
        } 

        //Now when the user types exit the program will break off from the loop and exit
        else if((strcmp("exit",t[0])==0))
        {
            printf("Now the file system is closed.\n");

            if(file_pointer!=NULL)
            {
                fclose(file_pointer);
                file_pointer=NULL;
            }
            break;
        }
            
        free( wr );
	
    }
	  return 0;
}

int LBAToOffset(int32_t s)
{
    return ((s - 2) * BPS) + (BPS * ReservedSC) + (FatsNum * Fz32 * BPS);
}

bool compare(char* IN, char* input)
{
  char EN[12];
  memset( EN, ' ', 12);

  char *t = strtok( input, "." );

  strncpy( EN, t, strlen( t ) );

  t = strtok( NULL, "." );

  if( t )
  {
    strncpy( (char*)(EN+8), t, strlen(t ) );
  }

  EN[11] = '\0';

  int i;
  for( i = 0; i < 11; i++ )
  {
    EN[i] = toupper( EN[i] );
  }
  if( strncmp( EN, IN, 11 ) == 0 )
  {
  	return true;
  }
  else
  {
	return false;
  }
}

int16_t NextLB(uint32_t s)
{
    uint32_t Address_of_FAT = (BPS*ReservedSC)+(s*4);
    fseek(file_pointer, Address_of_FAT, SEEK_SET);
    int16_t v;
    fread(&v, 2, 1, file_pointer);
    return v;
}
