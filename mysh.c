#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>

void ouch(int sig){ //Ctrl-Cの入力ありのとき
printf("read Ctrl+C. (got signal: %d)\n",sig);
(void) signal(SIGINT,ouch);
}

void Set_Path(){ //.pathを読んでコマンドサーチパスを指定できるようにするための関数。
int fd;
char buf[50];
 if ((fd = open(".path", O_RDONLY)) != -1) {  // 設定ファイルが開けた場合
//	printf("Success open\n");
    read(fd,buf,sizeof(buf));
    setenv("PATH",buf,1);
     }else {
	 printf("not succees\n");}
     close(fd);
}

//2入力のコマンドをつなげるためのパイプを作成する関数
void unite_pipe(char* cmd0[], char* cmd1[]){
int fd[2];
pid_t pid;
pipe(fd);
if( (pid=fork()) == 0){ //子
   dup2(fd[1],1);
   close(fd[0]); 
   close(fd[1]);
   execvp(cmd0[0],cmd0);
}
   dup2(fd[0],0); //親
   close(fd[0]);
   close(fd[1]);
   execvp(cmd1[0],cmd1);
}

//文字列を読んでベクタを作成する関数
int strtovec(char *s, char **v, int max){
int i=0,skip;
if(max < 1 || v == 0){skip=1;} 
else {skip=0;}
 while(1){
if(!skip && i >= max-1){v[i]=0; skip=1;}
while(*s != '\0' && isspace(*s)) s++; //終端でなく、空白文字の場合
if(*s=='\0') break; //文字の終端でやめる
if(!skip) v[i]=s; i++;
while(*s != '\0' && !isspace(*s)) s++; //終端でなく、空白でもない場合
if(*s=='\0') break;  //文字の終端でやめる
*s='\0'; s++;
}
if(!skip)
v[i]=0;
i++;
return i;
}

//改行文字までを読み捨てる関数
void discardline(FILE *fp){
int c;
while((c = getc(fp)) != EOF)
if(c =='\n'){break;}
}

//文字列を標準入力ストリームへ落とし込む関数
void getstr(char *prompt, char *s, int slen){
char *p,buf[50];
fputs(prompt,stderr); //myprommpt> の表示
fgets(buf,50,stdin); //コマンドの取り込み
if((p=strchr(buf,'\n')) == NULL){
discardline(stdin); //改行までの切捨
} else {*p='\0';}
if(strlen(buf) < slen){ //最大文字より少ない場合
strcpy(s,buf);
}else{
strncpy(s,buf,slen-1);
s[slen-1]='\0';
}
}

int main(void){
(void)signal(SIGINT,ouch); //Ctrl+c入力によるシグナルの処理
Set_Path();  //Pathの設定処理

char cmd[50];
char *ui[50],*p_av[50][50],*infile,*outfile;
int i,word,ac,status,pipes;
pid_t cpid;

while(1){
getstr("myprompt> ",cmd,sizeof(cmd)); //mypromptの表示、コマンド入力待ち
if(feof(stdin)){  //ストリームの終端(EOF)に達しているかどうか判定
    exit(0);
} else if(ferror(stdin)) {
    perror("getstr");
	exit(1);
  }
  if((word = strtovec(cmd,ui,100)) > 100){
      fputs("too many arguments\n",stderr);
	  continue;
	 }
  word--;
  if(word == 0){continue;} //改行処理
  if((strcmp(ui[0],"exit"))==0) break; //"exit"入力で終了
  
  
  // 入出力切り替え処理
  infile=NULL;outfile=NULL;ac=0,pipes=0;
  for(i=0;i<word;i++){
if (strcmp(ui[i],">") == 0){
  outfile = ui[i+1];
  i++;
  } else if(strcmp(ui[i],"<") == 0){
  infile = ui[i+1];
  i++;
  }
//同様にパイプで使用する '|'の判定  
    else if(strcmp(ui[i],"|") == 0){
	ac=0;
	p_av[++pipes][ac++] = ui[i+1];
	i++;
  } else {
    p_av[pipes][ac++] = ui[i];
  }
} 
    p_av[pipes][ac] = NULL; 
 
if((cpid=fork()) == -1){ //子プロセスの作成
   perror("fork");
   continue;
   } else if(cpid==0){ //cpid=0 子プロセス
   if(infile != NULL){
   close(0);
   if(open(infile,O_RDONLY)==-1){
   perror(infile);
   exit(1);
   }
   }
   if(outfile != NULL){
   close(1);
   if(open(outfile,O_WRONLY|O_CREAT|O_TRUNC,0666) == -1){
   perror(outfile);
   exit(1);
   }
   }
   for(i=0;i<pipes;i++){//パイプ
   unite_pipe(p_av[i],p_av[i+1]);
   }
   execvp(p_av[0][0],p_av[0]);
   if(strcmp(p_av[0][0],"exit")==0){break;}
   printf("myprompt> %s: Command Not Found\n",p_av[0][0]);
   exit(1);
} //子プロセス終   
   if(wait(&status)==(pid_t)-1){
   perror("wait");
   exit(1);
   }
   
   } //while-loop終
   exit(0);
   } //main
   