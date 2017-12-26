#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<stdio.h>
#include <string.h>

#define BUFFERSIZE 80

extern char *get_current_dir_name(void);
extern char *getenv(const char *name);
extern pid_t waitpid(pid_t pid, int *status, int options);

char buffer[BUFFERSIZE+1];

main()
{
    char *path, *arg[10], *input;
    int  li_inputlen, is_bj, is_back, i, j, k, pid, status;
    char lc_char;

    while (1){
    /* initiations */
        is_bj = 0; /*redirection flag*/
        is_back = 0; /*background*/

    /* shell prompt */
        path = get_current_dir_name();
        printf("%s>$",path);

    /*开始获取输入*/
        li_inputlen = 0;
        lc_char = getchar();
        while (lc_char !='\n'){
            if(li_inputlen < BUFFERSIZE)
	        buffer[li_inputlen++] = lc_char;
	    lc_char = getchar();
	}
	
	/*命令超长处理*/
	if (li_inputlen >= BUFFERSIZE){
	    printf("Your command is too long! Please re-enter your command!\n");
	    li_inputlen = 0;	/*reset */
	    continue;
	}
	else
	    buffer[li_inputlen] = '\0';/*加上串结束符号，形成字串*/

	/*将命令从缓存拷贝到input中*/
	input = (char *) malloc(sizeof(char) * (li_inputlen+1));
	strcpy(input,buffer);
	/* 获取命令和参数并保存在arg中*/
	for (i = 0,j = 0,k = 0;i <= li_inputlen;i++){
		/*管道和重定向单独处理*/
	    if (input[i] == '<' || input[i] == '>' || input[i] =='|'){
		  if (input[i] == '|')
			pipel(input,li_inputlen); 
		  else
			redirect(input,li_inputlen);
		  is_bj = 1;
                  break;
	    }
   /*处理空格、TAB和结束符。不用处理‘\n'，大家如果仔细分析前面的获取输入的程序的话，
    *不难发现回车符并没有写入buffer*/
	    if (input[i] == ' ' || input[i] =='\t' || input[i] == '\0'){
		if (j == 0) /*这个条件可以略去连在一起的多个空格或者tab*/
		    continue;
		else{
			buffer[j++] = '\0';
	//Added by lichun			
	if(strcmp(buffer,"cop")==0){				
		strcpy(buffer,"cp");				
		arg[0]=(char*) malloc(sizeof(char)*(j-1));
		strcpy(arg[0],buffer);			
	}			
	if(strcmp(buffer,"era")==0){				
		strcpy(buffer,"rm");				
		arg[0]=(char*) malloc(sizeof(char)*(j-1));
		strcpy(arg[0],buffer);			
	} 			
	if(strcmp(buffer,"dis")==0){				
		strcpy(buffer,"echo");				
		arg[0]=(char*) malloc(sizeof(char)*(j+1));
		strcpy(arg[0],buffer);			
	}			
	//Added end
	arg[k] = (char *) malloc(sizeof(char)*j);
			/*将指令或参数从缓存拷贝到arg中*/
			strcpy(arg[k],buffer);
		   j = 0;	/*准备取下一个参数*/
			k++;
		}
	    } 	
	    else{	
		/*如果字串最后是‘&'，则置后台运行标记为1*/
		if (input[i] == '&' && input[i+1] == '\0'){
		    is_back = 1;
		    continue;
		}
		buffer[j++] = input[i];
	    }	
        }
	free(input);/*释放空间*/

	/*如果输入的指令是leave则退出while，即退出程序*/
        if (strcmp(arg[0],"end") == 0 ){
	     printf("bye-bye\n");
	     break;
	}
	
	if (is_bj == 0){   /*非管道、重定向指令*/
/*在使用xxec执行命令的时候，最后的参数必须是NULL指针，
 *所以将最后一个参数置成空值*/
            arg[k] = (char *) 0;
		/*判断指令arg[0]是否存在*/
	    if (is_fileexist(arg[0]) == -1 ){

	         printf("This command is not found?!\n");
	         for(i=0;i<k;i++)
			free(arg[i]);
	         continue;
	    }

        /* fork a sub-process to run the execution file */
	    if ((pid = fork()) ==0) 		/*子进程*/
	     execv(buffer,arg);
	    else	/*父进程*/
	     if (is_back == 0)  /*并非后台执行指令*/
	         waitpid(pid,&status,0);
			
		  /*释放申请的空间*/
	    for (i=0;i<k;i++)
               free(arg[i]);
        }
    }
}

int is_fileexist(char *comm)
{
    char *path,*p;
    int i;

    i = 0;
/*使用getenv函数来获取系统环境变量，用参数PATH表示获取路径*/
    path = getenv("PATH");
    p = path;
    while (*p != '\0'){
		 /*路径列表使用‘:’来分隔路径*/
		 if (*p != ':')
          buffer[i++] = *p;
       else{
          buffer[i++] = '/';
          buffer[i] = '\0';
/*将指令和路径合成，形成pathname，并使用access函数来判断该文件是否存在*/
          strcat(buffer,comm);
		
          if (access(buffer,F_OK) == 0)	/*文件被找到*/
			 return 0;
          else	
			/*继续寻找其它路径*/
			 i = 0;
       }	
       p++; 
    }
/*搜索完所有路径，依然没有找到则返回-1*/
    return -1;
}

int redirect(char *in,int len)
{
    char *argv[30],*filename[2];
    pid_t pid;
    int i,j,k,fd_in,fd_out,is_in = -1,is_out = -1,num = 0;
    int is_back = 0,status=0;

/*这里是重定向的命令解析过程，其中filename用于存放重定向文件，
 *is_in, is_out分别是输入重定向标记和输出重定向标记*/
    for (i = 0,j = 0,k = 0;i <= len;i++){
        if(in[i]==''||in[i]=='\t'||in[i]=='\0'||in[i]=='<'||in[i]=='>'){
            if (in[i] == '>' || in[i] == '<'){
/*重定向指令最多'<'，'>'各出现一次，因此num最大为2，*否则认为命令输入错误*/
                if (num < 3){
	            num ++;
	            if (in[i] == '<') 
	                is_in = num - 1;
	            else
	                is_out = num - 1;
				
		   /*处理命令和重定向符号相连的情况，比如ls>a*/
		    if (j > 0 && num == 1)	{
		        buffer[j++] = '\0';
		        argv[k] = (char *) malloc(sizeof(char)*j);
		        strcpy(argv[k],buffer);
		        k++;
		        j = 0;
                    }
                }
	        else{
	            printf("The format is error!\n");
	            return -1;
	        }
            }	
	    if (j == 0) 
	        continue;
	    else{
		buffer[j++] = '\0';
		/*尚未遇到重定向符号，字符串是命令或参数*/
	if (num == 0){
		    argv[k] = (char *) malloc(sizeof(char)*j);
		    strcpy(argv[k],buffer);
		    k++;
		}
		/*是重定向后符号的字符串，是文件名*/
		else{
		    filename[status] = (char *) malloc(sizeof(char)*j);
		    strcpy(filename[status++],buffer);
		}
		j = 0;	/*initate*/
            }
        } 
        else{	
	    if (in[i] == '&' && in[i+1] == '\0'){
	        is_back = 1;
	        continue;
	    }
	    buffer[j++] = in[i];
        }	
    }

    argv[k] = (char *) 0;

    if (is_fileexist(argv[0]) == -1 ){
        printf("This command is not founded!\n");
        for(i=0;i<k;i++)
            free(argv[i]);
        return 0;
    }

    if ((pid = fork()) ==0){
		/*存在输出重定向*/
        if (is_out != -1)
            if((fd_out=open(filename[is_out],O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR))==-1){ 
             printf("Open out %s Error\n",filename[is_out]); 
             return -1; 
            } 

		/*存在输入重定向*/
        if (is_in != -1)
         if((fd_in=open(filename[is_in],O_RDONLY,S_IRUSR|S_IWUSR))==-1){ 
                printf("Open in %s Error\n",filename[is_out]); 
                return -1; 
            } 

        if (is_out != -1)
		 /*使用dup2函数将标准输出重定向到fd_out上，dup2(int oldfd,int newfd)实现的
	  *是把oldfd所指的文件描述符复制到newfd。若newfd为一已打开的文件描述词，
     *则newfd所指的文件会先被关闭，dup2复制的文件描述词与原来的文件描述词
     *共享各种文件状态*/
            if(dup2(fd_out,STDOUT_FILENO)==-1){ 
                printf("Redirect Standard Out Error\n"); 
                exit(1); 
            } 

	if (is_in != -1)
	    if(dup2(fd_in,STDIN_FILENO)==-1){ 
                printf("Redirect Standard Out Error\n"); 
                exit(1); 
	    } 
        execv(buffer,argv);
    }
    else
        if (is_back == 0)  /*run on the TOP*/
            waitpid(pid,&status,0);

    for (i=0;i<k;i++)
        free(argv[i]);

    if (is_in != -1){
        free(filename[is_in]);
        close(fd_in);
    }
    if (is_out != -1){
        free(filename[is_out]);
        close(fd_out);
    }
    return 0;
}

int pipel(char *input,int len)
{
  char *argv[2][30];
  int i,j,k,count,is_back = 0;
  int li_comm = 0,fd[2],fpip[2];
  char lc_char,lc_end[1];
  pid_t child1,child2;

/*管道的命令解析过程*/
  for (i = 0,j = 0,k = 0;i <= len;i++){
    if (input[i]== ' ' || input[i] == '\t' || input[i] == '\0' || input[i] == '|'){
      if (input[i] == '|' ) /*管道符号*/
      {
        if (j > 0)
        {
          buffer[j++] = '\0';
         /*因为管道连接的是两个指令，所以用二维数组指针来存放命令和参数，
          *li_comm是表示第几个指令*/
          argv[li_comm][k] = (char *) malloc(sizeof(char)*j);
          strcpy(argv[li_comm][k++],buffer);
        }
        argv[li_comm][k++] = (char *) 0;
        /*遇到管道符，第一个指令完毕，开始准备接受第二个指令*/
        li_comm++; 
        count = k;
        k=0;j=0;
      }
      if (j == 0)
        continue;
      else
      {
        buffer[j++] = '\0';
        argv[li_comm][k] = (char *) malloc(sizeof(char)*j);
        strcpy(argv[li_comm][k],buffer);
        k++;
      }
      j = 0;	/*initate*/
    }
    else{	
      if (input[i] == '&' && input[i+1] == '\0'){
        is_back = 1;
        continue;
      }
      buffer[j++] = input[i];
    }
  }
  argv[li_comm][k++] = (char *) 0;

  if (is_fileexist(argv[0][0]) == -1 ){
    printf("This first command is not found!\n");
    for(i=0;i<count;i++)
      free(argv[0][i]);
    return 0;
  }
/*指令解析结束*/

/*建立管道*/
  if (pipe(fd) == -1 ){
    printf("open pipe error!\n");
    return -1;
  }
/*创建第一个子进程执行管道符前的指令，并将输出写到管道*/
  if ((child1 = fork()) ==0){
/*关闭读端*/
    close(fd[0]);
    if (fd[1] != STDOUT_FILENO){
   /*将标准输出重定向到管道的写入端，这样该子进程的输出就写入了管道*/
      if (dup2(fd[1],STDOUT_FILENO) == -1){
        printf("Redirect Standard Out Error\n");
        return -1;
      }
   /*关闭写入端*/
      close(fd[1]);
    }
    execv(buffer,argv[0]);
  }
  else{           /*父进程*/
/*先要等待写入管道的进程结束*/
    waitpid(child1,&li_comm,0);	
/*然后我们必须写入一个结束标记，告诉读管道进程数据到这里就完了*/
    lc_end[0] = 0x1a;
    write(fd[1],lc_end,1);
    close(fd[1]);

    if (is_fileexist(argv[1][0]) == -1 ){
      printf("This command is not founded!\n");
      for(i=0;i<k;i++)
      free(argv[1][i]);
      return 0;
    }

/*创建第二个进程执行管道符后的指令，并从管道读输入流 */
    if ((child2 = fork()) == 0){
      if (fd[0] != STDIN_FILENO){
   /*将标准输入重定向到管道读入端*/
        if(dup2(fd[0],STDIN_FILENO) == -1){
          printf("Redirect Standard In Error!\n");
          return -1;
        }
        close(fd[0]);
      }
      execv(buffer,argv[1]);
    }
    else     /*父进程*/
      if (is_back == 0)
        waitpid(child2,NULL,0);
  }
  for (i=0;i<count;i++)
    free(argv[0][i]);
  for (i=0;i<k;i++)
    free(argv[1][i]);
  return 0;
}
