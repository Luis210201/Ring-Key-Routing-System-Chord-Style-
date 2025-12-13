#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#define PORT "58001"
#include<string.h>
#include <time.h>

typedef struct Node{
    int chave;
    char IP[16];
    char Port[9];
    char IP_prev[16];
    char Port_prev[9];
    int chave_prev;
    char IP_next[16];
    char Port_next[9];
    int chave_next;
    char IP_atalho[16];
    char Port_atalho[9];
    int chave_atalho;
    int isinring; //if its inside a ring then ==1,otherwise ==0
    int chave_procurada;
}Node;

int makeudpserv (Node no1){
    int selfd,n,errcode;
    struct addrinfo hints,*res;
    selfd=socket(AF_INET,SOCK_DGRAM, 0);
    if(selfd==1){
     exit(1);}
    memset(&hints, 0,sizeof hints);
    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_DGRAM;
    hints.ai_flags=AI_PASSIVE;
    errcode=getaddrinfo(NULL,no1.Port, &hints,&res);
    if(errcode!=0){
        exit(1);
    }
    n=bind(selfd,res->ai_addr, res->ai_addrlen);
    if(n==-1)
    {
        exit(1);}
    return selfd;
}
  
int maketcpserv (Node no1){
    struct addrinfo hints,*res;
    int fd,errcode;



    if((fd=socket(AF_INET,SOCK_STREAM,0))==-1)
    {
        exit(1);}//error
    memset(&hints,0,sizeof hints);

    hints.ai_family=AF_INET;//IPv4
    hints.ai_socktype=SOCK_STREAM;//TCP socket
    hints.ai_flags=AI_PASSIVE;
    if((errcode=getaddrinfo(NULL,no1.Port,&hints,&res))!=0){

            /*error*/ exit(1);}
    if(bind(fd,res->ai_addr,res->ai_addrlen)==-1){

            /*error*/ exit(1);}
    if(listen(fd,5)==-1){

            /*error*/exit(1);}
    return fd;
}

int make_acept(int fd, Node no){
    socklen_t addrlen;
    int newfd;

    struct sockaddr addr;
    addrlen=sizeof(no.IP);
        if((newfd=accept(fd,&addr,&addrlen))==-1)
            /*error*/exit(1);

    return newfd;
}

int maketcpclient(Node no){

    struct addrinfo hints,*res;
    int fd,n;
    fd=socket(AF_INET,SOCK_STREAM,0);//TCP socket
    if(fd==-1)exit(1);//error
    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET;//IPv4
    hints.ai_socktype=SOCK_STREAM;//TCP socket

    n=getaddrinfo(no.IP_prev,no.Port_prev,&hints,&res);
    if(n!=0){
        exit(1);
    }
    n=connect(fd,res->ai_addr,res->ai_addrlen);
    if(n==-1){
        exit(1);
    }

    return fd;
}

void writemensagetcp(int fd,char *mensage){

    char buffer[128];
    bzero(buffer,128); //garantir que o buffer esta vazio
    strcpy(buffer,mensage); //copiar a mensagem pro buffer
    write(fd,buffer,strlen(buffer)); //escrever a mensagem do buffer no fd
    bzero(buffer,128); //esvaziar o buffer
    return;
}

void writemensageudp(int fdu,char ip[18],char port[9],char mensage[128]){

    char buffer[128];
    struct addrinfo hints,*res;
    int errcode,confirmed=0;
    ssize_t n;
    memset(&hints, 0,sizeof hints);
    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_DGRAM;
    hints.ai_flags=AI_PASSIVE;
    bzero(buffer,128);
    strcpy(buffer,mensage);
    errcode=getaddrinfo(ip,port, &hints,&res);
    if(errcode!=0){
     exit(1);}
     struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 300000;
    if (setsockopt(fdu, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
    perror("Error");
}
    for(int i=0;i<5;i++){

        n=sendto(fdu,buffer,sizeof(buffer),0,res->ai_addr,res->ai_addrlen);
        if(n==-1)/*error*/exit(1);
        bzero(buffer,128);

            printf("waiting\n");
            n=recvfrom(fdu,buffer,sizeof(buffer),0,NULL,NULL);
            if(n>=0){
                printf("%s\n",buffer);
                bzero(buffer,128);
                confirmed=1;
            break;
           }
            else{

            }
    }
    if(confirmed==0){
        printf("couldent send mensage after 5 atempts\n");
    }
    return;
}

int calcula_distancias(int chave1, int chave_procurada){

    int distance;
    int N=32;

    if(chave_procurada>=chave1){  //serve de módulo
        distance = chave_procurada-chave1;
    }
    else{
        distance = N - (chave1-chave_procurada);
    }

    return distance;
}

int main(int argc,char *argv[])
{
    char in_str[128];
    srand(time(0));

    fd_set inputs, testfds;

    socklen_t udplen;


    struct sockaddr_in udpaddr;
    char buffer[128],ackbuf[128],*buffsuc,*buffpred,*buffsucnew,buffersuc[128],bufferpred[128],buffersucnew[128];
    char comando[20];
    struct Node no;
    if(argc!=4) //verificar se o numero de inputs esta correto
    {
        printf("Input is invalid\n");
        exit(0);
    }

    udplen=sizeof(udpaddr);
    no.chave=atoi (argv[1]);
    strcpy(no.IP,argv[2]);
    strcpy(no.Port,argv[3]);
    no.chave_next=-1;
    no.chave_prev=-1;
    no.chave_atalho=-1;
    no.isinring=0;


    char self[10];
    int chave,key,n,byteread,bytetotalsuc=0,bytetotalpred=0,bytetotalsucnew=0;
    char port[9],port_entry[9],portu[NI_MAXSERV];
    char ip[16],ip_entry[16];

    //As minhas declaraçoes para o find
    int id, key_proc, key_founder_proc, key_resp;
    int chave_find, identifier, distance_founder, distance_next, distance_atalho;
    int identifier_vec[99];

    int out_fds,fdu=-1,fdt=-1, fdsuc=-1, fdpred=-1 ,fdsuc_new=-1,servporttemp;

    char *ipserv;

    FD_ZERO(&inputs); // Clear inputs
    FD_SET(0,&inputs); // Set standard input channel on

    //printf("Size of fd_set: %d\n",sizeof(fd_set));
    //printf("Value of FD_SETSIZE: %d\n",FD_SETSIZE);

    for(int i = 0; i<100; i++){
        identifier_vec[i] = 0;
    }
    buffsuc=buffersuc;
    buffpred=bufferpred;
    buffsucnew=buffersucnew;
    fdu=makeudpserv(no);
    FD_SET(fdu,&inputs);
    fdt=maketcpserv(no);
    FD_SET(fdt,&inputs);

    while(1)
    {
        testfds=inputs;
      //  timeout.tv_sec=10000000000;
        //timeout.tv_usec=0;


        //printf("testfds byte: %d\n",((char *)&testfds)[0]);

        out_fds=select(FD_SETSIZE,&testfds,(fd_set *)NULL,(fd_set *)NULL,NULL);

        //printf("Time = %d and %d\n",timeout.tv_sec,timeout.tv_usec);

        //printf("testfds byte: %d\n",((char *)&testfds)[0]);

        switch(out_fds)
        {
        case 0:
            printf("Timeout event\n");
            break;
        case -1:
            perror("select");
            exit(1);
        default:
            if(FD_ISSET(0,&testfds))
            {
                if((n=read(0,in_str,127))!=0)
                {
                    if(n==-1)
                        exit(1);
                    in_str[n]=0;
                    chave=-1;
                    strcpy(port,"");
                    sscanf(in_str,"%s %d %s %s",order, &chave, ip, port);
                    if(strcmp(order,"new")==0){       //Meter aqui um switch de modo a reconhecer o que foi escrito no teclado e a realizar o comando que nos queremos
                        if(no.isinring==1){
                            printf("This program is already inserted in a ring\n");
                            break;
                        }
                        no.isinring=1;
                    }
                    else if(strcmp(order, "pentry")==0){
                        //para receber cenas cliente udp e tcp
                        if(no.isinring==1){ //garantir que nao pertence a nenhum ring
                            printf("This program is already inserted in a ring\n");
                            break;
                        }
                        strcpy(no.IP_prev,ip);
                        no.chave_prev = chave;
                        strcpy(no.Port_prev,port);
                        fdpred=maketcpclient(no); //obter a fd do processor
                        FD_SET(fdpred,&inputs);  //introduzila ao select
                        bzero(buffer,128); //garantir que o buffer esta vazio
                        sprintf(buffer,"SELF %d %s %s\n",no.chave,no.IP,no.Port); //escrever a instrução de SELF no buffer
                        writemensagetcp(fdpred,buffer); //escrever a mensagem para o no anterior
                        bzero(buffer,128); //esvaziar o buffer
                        no.isinring=1;

                    }
                    else if(strcmp(order, "show")==0){
                        if((no.chave_next==-1)&&(no.chave_prev==-1)&&(no.isinring==0))
                            printf("Status:\n\nSelf:\nkey=%d IP=%s Port=%s\nPredecessor:\nkey=None IP=None Port=None\nSuccessor:\nkey=None IP=None Port=None\nShortcut:\nkey=None IP=None Port=None\n",no.chave,no.IP,no.Port);
                        else if((no.chave_next==-1)&&(no.chave_prev==-1))
                            printf("Status:\nSelf:\nkey=%d IP=%s Port=%s\nPredecessor:\nkey=%d IP=%s Port=%s\nSuccessor:\nkey=%d IP=%s Port=%s\nShortcut:\nkey=None IP=None Port=None\n",no.chave,no.IP,no.Port,no.chave,no.IP,no.Port,no.chave,no.IP,no.Port);
                        else if(no.chave_atalho==-1)
                            printf("Status:\nSelf:\nkey=%d IP=%s Port=%s\nPredecessor:\nkey=%d IP=%s Port=%s\nSuccessor:\nkey=%d IP=%s Port=%s\nShortcut:\nkey=None IP=None Port=None\n",no.chave,no.IP,no.Port,no.chave_prev,no.IP_prev,no.Port_prev,no.chave_next,no.IP_next,no.Port_next);
                        else
                        printf("Status:\nSelf:\nkey=%d IP=%s Port=%s\nPredecessor:\nkey=%d IP=%s Port=%s\nSuccessor:\nkey=%d IP=%s Port=%s\nShortcut:\nkey=%d IP=%s Port=%s\n",no.chave,no.IP,no.Port,no.chave_prev,no.IP_prev,no.Port_prev,no.chave_next,no.IP_next,no.Port_next,no.chave_atalho,no.IP_atalho,no.Port_atalho);
                    }
                    else if(strcmp(order, "leave")==0){
                        bzero(buffer,128); //garantir que o buffer esta vazio
                        sprintf(buffer,"PRED %d %s %s\n",no.chave_prev,no.IP_prev,no.Port_prev); //escrever a instrução de SELF no buffer
                        writemensagetcp(fdsuc,buffer); //escrever a mensagem para o no anterior
                        bzero(buffer,128); //esvaziar o buffer
                        FD_CLR(fdsuc,&inputs);
                        FD_CLR(fdpred,&inputs);
                        no.chave_next=-1;
                        no.chave_prev=-1;
                        no.isinring=0;
                    }
                    else if(strcmp(order, "chord")==0){
                        if(strcmp(port,"")==0){
                            printf("invalid input, please insert a key,ip and port\n");
                            break;
                        }
                        if(no.isinring==0){
                            printf("this node isint inside the ring, please enter it using the instruction pentry or bentry\n");
                            break;
                        }
                        if(no.chave_atalho!=-1){
                            printf("this node already has a shortcut, please remove it by using the instruction echord\n");
                            break;
                        }
                        else{ //praticamente so isto é que importa
                            no.chave_atalho=chave;
                            strcpy(no.IP_atalho,ip);
                            strcpy(no.Port_atalho,port);
                        }
                    }
                    else if(strcmp(order, "help")==0){
                        printf("Lista de comandos que podem ser chamados:\n\n");
                        printf("- new ---> comando para criar o anel\n");
                        printf("- pentry ---> comando que permite um nó entrar no anel recebendo como parâmetros chave, ip e port do nó que vai ser o seu predecessor\n\n");
                        printf("- chord ---> comando que cria um atalho entre o nó atual e o nó que é recebido como parâmetro\n\n");
                        printf("- echord---> comando que apaga um atalho\n\n");
                        printf("- leave ---> comando que efetua a saída de um nó do anel\n\n");
                        printf("- find ---> comando que efetua uma pesquisa ao longo do nó de modo a encontrar a chave que recebe como parâmetro\n\n");
                        printf("- bentry ---> comando que efetua uma pesquisa para encontrar a localização onde o nó(que é recbido como parâmetro) vai ser inserido e vai realizar a inserção do mesmo\n\n");
                        printf("- exit ---> fecho de todas as ligações de um nó\n\n");
                    }
                    else if(strcmp(order, "echord")==0){
                        if(no.isinring==0){
                            printf("this node isint inside the ring, please enter it using the instruction pentry or bentry\n");
                            break;
                        }
                        if(no.chave_atalho==-1){
                            printf("there is no shortcut on this node");
                            break;
                        }
                        else{
                            no.chave_atalho=-1;
                        }
                    }
                    else if(strcmp(order, "find")==0){

                        //meter aqui que so sao permitidas chaves ente 0 e 32
                        if(chave <0 || chave>31){
                            printf("por favor introduza uma chave entre 0 e 31\n");
                            break;
                        }

                        //verificar que o no ta sozinho ent nao da para procurar
                        if(no.chave_next == -1 && no.chave_prev ==-1){
                            printf("Chave %d: nó %d(%s:%s)\n", chave, no.chave, no.IP, no.Port);
                            break;
                        }

                        no.chave_procurada = chave;
                        chave_find = chave;
                        identifier = rand()%100;

                        identifier_vec[identifier] = 0;

                        distance_founder = calcula_distancias(no.chave, chave_find); //distancia do próprio nó ao nó procurado
                        distance_next = calcula_distancias(no.chave_next, chave_find);//distancia do next nó ao nó procurado


                        if(no.chave_atalho != -1){ // verificar a existência de atalho

                            distance_atalho = calcula_distancias(no.chave_atalho, chave_find);//distancia do nó_atalho ao nó procurado


                            if(distance_founder > distance_atalho){//verifica se o nó que procurado está fora do caminho entre o nó atual e o atalho

                                bzero(buffer,128);
                                sprintf(buffer,"FND %d %d %d %s %s\n", chave_find, identifier, no.chave,no.IP,no.Port); //escrever a instrução FND para o próximo nó
                                writemensageudp(fdu,no.IP_atalho,no.Port_atalho, buffer); //escrever a mensagem para o no seguinte ------> este novo tipo de mensagem tem de mandar tb alguma informaçao sobre o no que contacta qual
                                bzero(buffer,128); //esvaziar o buffer

                            }
                            else{

                                if(distance_founder > distance_next){//verifica se o nó procurado não está entre o nó atual e o next

                                    bzero(buffer,128);
                                    sprintf(buffer,"FND %d %d %d %s %s\n", chave_find, identifier, no.chave,no.IP,no.Port); //escrever a instrução de SELF no buffer
                                    writemensagetcp(fdsuc,buffer); //escrever a mensagem para o no seguinte;
                                    bzero(buffer,128); //esvaziar o buffer

                                }
                                else{
                                    printf("Chave %d: nó %d(%s:%s)\n", no.chave_procurada, no.chave, no.IP, no.Port);//resposta imediata pois nao precisa sequer de sair do nó original
                                }
                            }
                        }
                        else{



                            if(distance_founder > distance_next){//verifica se o nó procurado não está entre o nó atual e o next

                                bzero(buffer,128);
                                sprintf(buffer,"FND %d %d %d %s %s\n", chave_find, identifier, no.chave,no.IP,no.Port); //escrever a instrução FND para o próximo nó
                                writemensagetcp(fdsuc,buffer); //escrever a mensagem para o no seguinte;
                                bzero(buffer,128); //esvaziar o buffer

                            }
                            else{
                                printf("Chave %d: nó %d(%s:%s)\n", no.chave_procurada, no.chave, no.IP, no.Port);// resposta imediata pois nao precisa sequer de sair do nó original
                            }
                        }

                    }
                    else if(strcmp(order, "bentry")==0){
                        //AQUI VAMOS TER DE CRIAR UM WRITEMENSAGEUDP NOVO OU ENT ALTERAR SO A MANEIRA COMO RECEBE OS ARGUMENTOS
                        //POIS AQUI O NO Q VAI SER DELEGADO NAO TA NA ESTRUTURA
                        if(no.isinring==1){ //garantir que nao pertence a nenhum ring
                            printf("This program is already inserted in a ring\n");
                            break;
                        }
                        if((chave<0) || (chave>31)){
                            printf("por favor introduza uma chave de procura entre 0 e 31\n");
                            break;
                        }
                        bzero(buffer,128);
                        sprintf(buffer,"EFND %d", no.chave); //escrever a instrução FND para o próximo nó
                        writemensageudp(fdu,ip,port,buffer); //escrever a mensagem para o nó que se quer delegar a procura de onde vai entrar; no especial deste
                        bzero(buffer,128); //esvaziar o buffer

                    }
                    else if(strcmp(order, "exit")==0){
                        if(no.isinring==0){ //caso em que é apenas um no sozinho
                            close(fdpred);
                            close(fdsuc);
                            close(fdsuc_new);
                            close(fdt);
                            close(fdu);
                            exit(0);
                        }
                        else if(no.isinring==1){ //caso em que esta dentro de um ring ainda com ligacoes por acabar
                            printf("Please exit the ring first, this can be done using the command leave.\n");
                            break;
                        }
                        else{
                            close(fdpred);
                            close(fdsuc);
                            close(fdsuc_new);
                            close(fdt);
                            close(fdu);
                            exit(0);
                        }
                    }
                    else if(order=='\0'){
                    }
                    else{
                        printf("Invalid input\n");
                    }

                }
            }
            if(FD_ISSET(fdu,&testfds))
            {
                if((n=recvfrom(fdu,buffer,127,0,(struct sockaddr *)&udpaddr,&udplen))) //ele recebe SEMPRE as mensagens udp por aquim o buffer tem a mensagem recebida
                {
                    if(n==-1){
                        exit(1);
                    }
                    buffer[n]='\0';
                    bzero(ackbuf,128);
                    strcpy(ackbuf,"ACK");
                    sendto(fdu,ackbuf,strlen(ackbuf),0,(struct sockaddr *)&udpaddr,udplen);
                    bzero(ackbuf,128);

                    sscanf(buffer, "%s", self);

                    if(strcmp(self, "FND")==0){

                        sscanf(buffer, "%s %d %d %d %s %s", self, &key_proc, &id, &key, ip_entry, port_entry);
                        bzero(buffer,128);


                        distance_founder = calcula_distancias(no.chave, key_proc); //distancia do próprio nó ao nó procurado
                        distance_next = calcula_distancias(no.chave_next, key_proc);//distancia do next nó ao nó procurado


                        if(no.chave_atalho != -1){//verifica se possui algum atalho ou não

                            distance_atalho = calcula_distancias(no.chave_atalho, key_proc);


                            if(distance_founder > distance_atalho){//verifica se o nó procurado não está entre o nó atual e o next



                                bzero(buffer,128);
                                sprintf(buffer,"FND %d %d %d %s %s", key_proc, id, key, ip_entry, port_entry); //escrever a instrução FND no buffer
                                writemensageudp(fdu,no.IP_atalho,no.Port_atalho, buffer); //escrever a mensagem para o próximo nó
                                bzero(buffer,128); //esvaziar o buffer

                            }
                            else{

                                if(distance_founder > distance_next){//verifica se o nó procurado não está entre o nó atual e o next

                                    bzero(buffer,128);
                                    sprintf(buffer,"FND %d %d %d %s %s\n", key_proc, id, key, ip_entry, port_entry); //escrever a instrução de SELF no buffer
                                    writemensagetcp(fdsuc,buffer); //escrever a mensagem para o no seguinte;
                                    bzero(buffer,128); //esvaziar o buffer

                                }
                                else{
                                    //SEND RESP
                                    //Prints so para ajudar no find
                                    if(distance_atalho<distance_next){
                                        bzero(buffer,128);
                                        sprintf(buffer,"RSP %d %d %d %s %s", key, id, no.chave, no.IP, no.Port); //escrever a instrução RSP no buffer
                                        writemensageudp(fdu,no.IP_atalho,no.Port_atalho, buffer); //escrever a mensagem para o próximo nó
                                        bzero(buffer,128); //esvaziar o buffer
                                    }

                                    else{

                                        bzero(buffer,128);
                                        sprintf(buffer,"RSP %d %d %d %s %s\n", key, id, no.chave, no.IP, no.Port); //escrever a instrução RSP no buffer
                                        writemensagetcp(fdsuc,buffer); //escrever a mensagem para o no seguinte;
                                        bzero(buffer,128); //esvaziar o buffer
                                    }



                                }
                            }
                        }

                       else if(distance_founder > distance_next){

                            bzero(buffer,128);
                            sprintf(buffer,"FND %d %d %d %s %s\n", key_proc, id, key, ip_entry, port_entry); //escrever a instrução FND no buffer
                            writemensagetcp(fdsuc,buffer); //escrever a mensagem para o no seguinte;
                            bzero(buffer,128); //esvaziar o buffer

                        }
                        else{
                            //SEND RESP


                            bzero(buffer,128);
                            sprintf(buffer,"RSP %d %d %d %s %s\n", key, id, no.chave, no.IP, no.Port); //escrever a instrução RSP no buffer
                            writemensagetcp(fdsuc,buffer); //escrever a mensagem para o no seguinte;
                            bzero(buffer,128); //esvaziar o buffer
                        }

                    }
                    else if(strcmp(self, "EFND")==0){

                        //vetor com 100 posiçoes e vou meter 1 la para a posiçao do random
                        servporttemp=ntohs(udpaddr.sin_port);
                        sprintf(portu,"%d",servporttemp);
                        ipserv = inet_ntoa(udpaddr.sin_addr);
                        identifier = rand()%100;
                        sscanf(buffer, "%s %d", order, &key_proc);
                        identifier_vec[identifier]=1;

                        //verificar que o no ta sozinho ent nao da para procurar
                        if((no.chave_next == -1) || (no.chave_prev ==-1)){
                            bzero(buffer,128);
                            sprintf(buffer,"EPRED %d %s %s",no.chave,no.IP,no.Port);
                            writemensageudp(fdu,ipserv,portu,buffer);
                            bzero(buffer,128);
                            break;
                        }

                        identifier_vec[identifier] = 1;

                        //dps aqui é fazer um find normal já
                        //o nó que recebe o a ordem para procurar vai mandar a procura

                        distance_founder = calcula_distancias(no.chave, key_proc); //distancia do próprio nó ao nó procurado
                        distance_next = calcula_distancias(no.chave_next, key_proc);//distancia do next nó ao nó procurado

                        if(no.chave_atalho != -1){//verifica se possui algum atalho ou não

                            distance_atalho = calcula_distancias(no.chave_atalho, key_proc);


                            if(distance_founder > distance_atalho){//verifica se o nó procurado não está entre o nó atual e o next



                                bzero(buffer,128);
                                sprintf(buffer,"FND %d %d %d %s %s", key_proc, identifier, no.chave, no.IP, no.Port); //escrever a instrução FND no buffer
                                writemensageudp(fdu,no.IP_atalho,no.Port_atalho, buffer); //escrever a mensagem para o próximo nó
                                bzero(buffer,128); //esvaziar o buffer

                            }
                            else{

                                if(distance_founder > distance_next){//verifica se o nó procurado não está entre o nó atual e o next

                                    bzero(buffer,128);
                                    sprintf(buffer,"FND %d %d %d %s %s\n", key_proc, identifier, no.chave, no.IP, no.Port); //escrever a instrução de SELF no buffer
                                    writemensagetcp(fdsuc,buffer); //escrever a mensagem para o no seguinte;
                                    bzero(buffer,128); //esvaziar o buffer

                                }
                                else{
                                    //SEND RESP
                                    //Prints so para ajudar no find


                                    bzero(buffer,128);
                                    sprintf(buffer,"RSP %d %d %d %s %s\n", key, identifier, no.chave, no.IP, no.Port); //escrever a instrução RSP no buffer
                                    writemensagetcp(fdsuc,buffer); //escrever a mensagem para o no seguinte;
                                    bzero(buffer,128); //esvaziar o buffer

                                }
                            }
                        }
                        else if(distance_founder > distance_next){


                        bzero(buffer,128);
                        sprintf(buffer,"FND %d %d %d %s %s\n", key_proc, identifier, no.chave, no.IP, no.Port); //escrever a instrução FND no buffer
                        writemensagetcp(fdsuc,buffer); //escrever a mensagem para o no seguinte;
                        bzero(buffer,128); //esvaziar o buffer

                    }
                    else{
                        //SEND RESP



                        bzero(buffer,128);
                        sprintf(buffer,"RSP %d %d %d %s %s\n", no.chave, identifier, no.chave, no.IP, no.Port); //escrever a instrução RSP no buffer
                        writemensagetcp(fdsuc,buffer); //escrever a mensagem para o no seguinte;
                        bzero(buffer,128); //esvaziar o buffer
                    }

                    }
                    else if(strcmp(self, "RSP")==0){

                        sscanf(buffer, "%s %d %d %d %s %s", self, &key_founder_proc, &id, &key_resp, ip_entry, port_entry);
                        bzero(buffer,128);

                        distance_founder = calcula_distancias(no.chave, key_founder_proc);// calcula as distâncias necessárias
                        distance_next = calcula_distancias(no.chave_next, key_founder_proc);


                        if(no.chave_atalho != -1){//verifica se tem algum atalho

                            distance_atalho = calcula_distancias(no.chave_atalho, key_founder_proc);



                            if(distance_founder > distance_atalho){//verifica se o nó não está entre o nó atual e o next



                                bzero(buffer,128);
                                sprintf(buffer,"RSP %d %d %d %s %s\n", key_founder_proc, id, key_resp, ip_entry, port_entry); //escrever a instrução de RSP no buffer
                                writemensageudp(fdu,no.IP_atalho,no.Port_atalho, buffer); //escrever a mensagem para o nó seguinte;
                                bzero(buffer,128); //esvaziar o buffer

                            }
                            else{

                                if(distance_founder > distance_next){

                                    bzero(buffer,128);
                                    sprintf(buffer,"RSP %d %d %d %s %s\n", key_founder_proc, id,key_resp, ip_entry, port_entry); //escrever a instrução RSP no buffer
                                    writemensagetcp(fdsuc,buffer); //escrever a mensagem para o nó seguinte;
                                    bzero(buffer,128); //esvaziar o buffer

                                }
                                else{
                                    if(identifier_vec[id]==1){
                                        bzero(buffer,128);
                                        sprintf(buffer,"EPRED %d %s %s",no.chave,no.IP,no.Port);
                                        writemensageudp(fdu,ipserv,portu,buffer);
                                        bzero(buffer,128);
                                    }
                                    else if(identifier_vec[id]==0){
                                        printf("Chave %d: no %d(%s:%s)\n", no.chave_procurada, key_resp, ip_entry, port_entry);//Localização exata do nó procurado
                                    }
                                }
                            }
                        }

                        else if(distance_founder > distance_next){

                            bzero(buffer,128);
                            sprintf(buffer,"RSP %d %d %d %s %s\n", key_founder_proc, id,key_resp, ip_entry, port_entry); //escrever a instrução
                            writemensagetcp(fdsuc,buffer); //escrever a mensagem para o nó seguinte;
                            bzero(buffer,128); //esvaziar o buffer
                        }
                        else{

                            if(identifier_vec[id]==1){
                                    bzero(buffer,128);
                                    sprintf(buffer,"EPRED %d %s %s",no.chave,no.IP,no.Port);
                                    writemensageudp(fdu,ipserv,portu,buffer);
                                    bzero(buffer,128);
                            }
                            else if(identifier_vec[id]==0){
                                printf("Chave %d: no %d(%s:%s)\n", no.chave_procurada, key_resp, ip_entry, port_entry);//Localização exata do nó procurado
                            }
                            /*
                            bzero(buffer,128);
                            sprintf(buffer,"EPRED %d %s %s\n", key_resp, ip_entry, port_entry); //escrever a mensagem para o nó que estava a espera de entrar no anel---> informaçao do predecessor
                            writemensageudp(fdsucudp,buffer); //escrever a mensagem para o no que quer entrar
                            bzero(buffer,128); //esvaziar o buffer*/
                        }

                    }

                else if(strcmp(self, "EPRED")==0){
                    sscanf(buffer, "%s %d %s %s", self, &key, ip_entry, port_entry);
                    strcpy(no.IP_prev,ip_entry);
                    no.chave_prev = key;
                    strcpy(no.Port_prev,port_entry);
                    fdpred=maketcpclient(no); //obter a fd do processor
                    FD_SET(fdpred,&inputs);  //introduzila ao select
                    bzero(buffer,128); //garantir que o buffer esta vazio
                    sprintf(buffer,"SELF %d %s %s\n",no.chave,no.IP,no.Port); //escrever a instrução de SELF no buffer
                    writemensagetcp(fdpred,buffer); //escrever a mensagem para o no anterior
                    bzero(buffer,128); //esvaziar o buffer
                    no.isinring=1;
                }
                }
            }
            if(FD_ISSET(fdt,&testfds)){
                    if(no.chave_next==-1){
                        fdsuc = make_acept(fdt, no);
                        FD_SET(fdsuc,&inputs);

                    }
                    else{
                        fdsuc_new = make_acept(fdt, no);
                        FD_SET(fdsuc_new, &inputs);


                    }

            }
            if(FD_ISSET(fdsuc,&testfds)){ //o sucessor apenas comunica com com SELF ou PRED, ou seja na entrada ou saida de um nó
                byteread=read(fdsuc,buffsuc,sizeof(buffersuc)); //ler o que esta em newfd para o buffer
                bytetotalsuc+=byteread;
                if(buffersuc[bytetotalsuc-1]!='\n'){
                    buffsuc+=byteread;

                    break;
                }
                buffersuc[bytetotalsuc]='\0';
                bytetotalsuc=0;

                buffsuc=buffersuc;

                sscanf(buffersuc,"%s", self);

                if(strcmp(self,"SELF")==0){//para realizar as funçoes certas em self

                    sscanf(buffersuc, "%s %d %s %s", self, &key, ip_entry, port_entry);

                    no.chave_next=key;
                    strcpy(no.IP_next,ip_entry);
                    strcpy(no.Port_next,port_entry);
                    if(no.chave_prev==-1){//se este no nao conter um prev quer dizer que esta sozinho(tem de ser o prev pq ele ja recebeu um self a dizer qual vai ser o sucessor)
                        strcpy(no.IP_prev,ip_entry);
                        strcpy(no.Port_prev,port_entry);
                        fdpred=maketcpclient(no); //obter a fd do processor
                        no.chave_prev=key;
                        FD_SET(fdpred,&inputs);  //introduzila ao select
                        bzero(buffer,128); //garantir que o buffer esta vazio
                        sprintf(buffer,"SELF %d %s %s\n",no.chave,no.IP,no.Port);
                        writemensagetcp(fdpred,buffer); //escrever a mensagem para o no anterior para este saber qual vai ser o sucessor(sendo que vai ser este no)
                    }
                    else{
                    }
                }
                if(strcmp(self,"PRED")==0){

                    sscanf(buffersuc, "%s %d %s %s", self, &key, ip_entry, port_entry);

                    no.chave_prev=key;
                    strcpy(no.IP_prev,ip_entry);
                    strcpy(no.Port_prev,port_entry);
                    FD_CLR(fdpred,&inputs);
                    fdpred=maketcpclient(no); //obter a fd do novo predecessor
                    FD_SET(fdpred,&inputs);  //introduzila ao select
                    bzero(buffer,128); //garantir que o buffer esta vazio
                    sprintf(buffer,"SELF %d %s %s\n",no.chave,no.IP,no.Port); //escrever a instruçao PRED no buffer
                    writemensagetcp(fdpred,buffer); //escrever a mensagem para o no sucessor antigo

                }
                bzero(buffersuc,128); //fazer reset ao buffer
            }
            if(FD_ISSET(fdsuc_new,&testfds)){

                byteread=read(fdsuc_new,buffsucnew,sizeof(buffersucnew)); //ler o que esta em newfd para o buffer
                bytetotalsucnew+=byteread;
                if(buffersucnew[bytetotalsucnew-1]!='\n'){
                    buffsucnew+=byteread;

                    break;
                }
                buffersucnew[bytetotalsucnew]='\0';
                bytetotalsucnew=0;

                buffsucnew=buffersucnew;

                sscanf(buffersucnew, "%s %d %s %s", self, &key, ip_entry, port_entry);

                if(strcmp(self,"SELF")==0){//para realizar as funçoes certas em self

                    if(calcula_distancias(no.chave,no.chave_next)>calcula_distancias(no.chave,key)){
                        no.chave_next=key;
                        strcpy(no.IP_next,ip_entry);
                        strcpy(no.Port_next,port_entry);
                        bzero(buffer,128); //garantir que o buffer esta vazio
                        sprintf(buffer,"PRED %d %s %s\n",no.chave_next,no.IP_next,no.Port_next); //escrever a instruçao PRED no buffer
                        writemensagetcp(fdsuc,buffer); //escrever a mensagem para o no sucessor antigo
                        FD_CLR(fdsuc,&inputs);
                        fdsuc = fdsuc_new;
                        FD_SET(fdsuc,&inputs);
                        FD_CLR(fdsuc_new,&inputs);
                        fdsuc_new=-1;
                    }
                    else{
                        no.chave_next=key;
                        strcpy(no.IP_next,ip_entry);
                        strcpy(no.Port_next,port_entry);
                        FD_CLR(fdsuc,&inputs);
                        fdsuc = fdsuc_new;
                        FD_SET(fdsuc,&inputs);
                        FD_CLR(fdsuc_new,&inputs);
                        fdsuc_new=-1;
                    }
                }
                bzero(buffersucnew,128); //fazer reset ao buffer


            }
            if(FD_ISSET(fdpred,&testfds)){

                byteread=read(fdpred,buffpred,sizeof(bufferpred)); //ler o que esta em newfd para o buffer
                bytetotalpred+=byteread;
                if(bufferpred[bytetotalpred-1]!='\n'){
                    buffpred+=byteread;
                    break;
                }
                bufferpred[bytetotalpred]='\0';
                bytetotalpred=0;
                buffpred=bufferpred;

                sscanf(bufferpred,"%s", self);

                if(strcmp(self,"PRED")==0){//para realizar as funçoes certas em self

                    sscanf(bufferpred, "%s %d %s %s", self, &key, ip_entry, port_entry);

                    if(no.chave==key){
                        FD_CLR(fdsuc,&inputs);
                        FD_CLR(fdpred,&inputs);
                        no.chave_next=-1;
                        no.chave_prev=-1;
                        no.isinring=1;
                    }
                    else{

                        sscanf(bufferpred, "%s %d %s %s", self, &key, ip_entry, port_entry);

                        no.chave_prev=key;
                        strcpy(no.IP_prev,ip_entry);
                        strcpy(no.Port_prev,port_entry);
                        FD_CLR(fdpred,&inputs);
                        fdpred=maketcpclient(no); //obter a fd do novo predecessor
                        FD_SET(fdpred,&inputs);  //introduzila ao select
                        bzero(buffer,128); //garantir que o buffer esta vazio
                        sprintf(buffer,"SELF %d %s %s\n",no.chave,no.IP,no.Port); //escrever a instruçao PRED no buffer
                        writemensagetcp(fdpred,buffer); //escrever a mensagem para o no sucessor antigo
                    }
                }
                else if(strcmp(self, "FND")==0){

                    sscanf(bufferpred, "%s %d %d %d %s %s", self, &key_proc, &id, &key, ip_entry, port_entry);
                    bzero(buffer,128);


                    distance_founder = calcula_distancias(no.chave, key_proc); //distancia do próprio nó ao nó procurado
                    distance_next = calcula_distancias(no.chave_next, key_proc);//distancia do next nó ao nó procurado


                    if(no.chave_atalho != -1){//verifica se possui algum atalho ou não

                        distance_atalho = calcula_distancias(no.chave_atalho, key_proc);


                        if(distance_founder > distance_atalho){//verifica se o nó procurado não está entre o nó atual e o next


                            bzero(buffer,128);
                            sprintf(buffer,"FND %d %d %d %s %s\n", key_proc, id, key, ip_entry, port_entry); //escrever a instrução FND no buffer
                            writemensageudp(fdu,no.IP_atalho,no.Port_atalho, buffer); //escrever a mensagem para o próximo nó
                            bzero(buffer,128); //esvaziar o buffer

                        }
                        else{

                            if(distance_founder > distance_next){//verifica se o nó procurado não está entre o nó atual e o next


                                bzero(buffer,128);
                                sprintf(buffer,"FND %d %d %d %s %s\n", key_proc, id, key, ip_entry, port_entry); //escrever a instrução de SELF no buffer
                                writemensagetcp(fdsuc,buffer); //escrever a mensagem para o no seguinte;
                                bzero(buffer,128); //esvaziar o buffer

                            }
                            else{
                                //SEND RESP
                                //Prints so para ajudar no find
                                if(distance_atalho<distance_next){
                                        bzero(buffer,128);
                                        sprintf(buffer,"RSP %d %d %d %s %s", key, id, no.chave, no.IP, no.Port); //escrever a instrução RSP no buffer
                                        writemensageudp(fdu,no.IP_atalho,no.Port_atalho, buffer); //escrever a mensagem para o próximo nó
                                        bzero(buffer,128); //esvaziar o buffer
                                }
                                else{
                                bzero(buffer,128);
                                sprintf(buffer,"RSP %d %d %d %s %s\n", key, id, no.chave, no.IP, no.Port); //escrever a instrução RSP no buffer
                                writemensagetcp(fdsuc,buffer); //escrever a mensagem para o no seguinte;
                                bzero(buffer,128); //esvaziar o buffer
                                }


                            }
                        }
                    }

                    else if(distance_founder > distance_next){



                        bzero(buffer,128);
                        sprintf(buffer,"FND %d %d %d %s %s\n", key_proc, id, key, ip_entry, port_entry); //escrever a instrução FND no buffer
                        writemensagetcp(fdsuc,buffer); //escrever a mensagem para o no seguinte;
                        bzero(buffer,128); //esvaziar o buffer

                    }
                    else{
                        //SEND RESP



                        bzero(buffer,128);
                        sprintf(buffer,"RSP %d %d %d %s %s\n", key, id, no.chave, no.IP, no.Port); //escrever a instrução RSP no buffer
                        writemensagetcp(fdsuc,buffer); //escrever a mensagem para o no seguinte;
                        bzero(buffer,128); //esvaziar o buffer
                    }

                }
                else if(strcmp(self, "RSP") == 0){

                    sscanf(bufferpred, "%s %d %d %d %s %s", self, &key_founder_proc, &id, &key_resp, ip_entry, port_entry);
                    bzero(buffer,128);

                    distance_founder = calcula_distancias(no.chave, key_founder_proc);// calcula as distâncias necessárias
                    distance_next = calcula_distancias(no.chave_next, key_founder_proc);



                    if(no.chave_atalho != -1){//verifica se tem algum atalho



                        distance_atalho = calcula_distancias(no.chave_atalho, key_founder_proc);



                        if(distance_founder > distance_atalho){//verifica se o nó não está entre o nó atual e o next

                            bzero(buffer,128);
                            sprintf(buffer,"RSP %d %d %d %s %s\n", key_founder_proc, id, key_resp, ip_entry, port_entry); //escrever a instrução de RSP no buffer
                            writemensageudp(fdu,no.IP_atalho,no.Port_atalho, buffer); //escrever a mensagem para o nó seguinte;
                            bzero(buffer,128); //esvaziar o buffer

                        }
                        else{

                            if(distance_founder > distance_next){

                                bzero(buffer,128);
                                sprintf(buffer,"RSP %d %d %d %s %s\n", key_founder_proc, id,key_resp, ip_entry, port_entry); //escrever a instrução RSP no buffer
                                writemensagetcp(fdsuc,buffer); //escrever a mensagem para o nó seguinte;
                                bzero(buffer,128); //esvaziar o buffer

                            }
                            else{
                                if(identifier_vec[id]==1){
                                    bzero(buffer,128);
                                    sprintf(buffer,"EPRED %d %s %s",key_resp,ip_entry,port_entry);
                                    writemensageudp(fdu,ipserv,portu,buffer);
                                    bzero(buffer,128);
                                }
                                else if(identifier_vec[id]==0){
                                    printf("Chave %d: no %d(%s:%s)\n", no.chave_procurada, key_resp, ip_entry, port_entry);//Localização exata do nó procurado
                                }
                            }
                        }
                    }
                    else{

                        if(distance_founder > distance_next){

                            bzero(buffer,128);
                            sprintf(buffer,"RSP %d %d %d %s %s\n", key_founder_proc, id,key_resp, ip_entry, port_entry); //escrever a instrução
                            writemensagetcp(fdsuc,buffer); //escrever a mensagem para o nó seguinte;
                            bzero(buffer,128); //esvaziar o buffer
                        }
                        else{

                            if(identifier_vec[id]==1){
                                bzero(buffer,128);
                                sprintf(buffer,"EPRED %d %s %s",key_resp,ip_entry,port_entry);
                                writemensageudp(fdu,ipserv,portu,buffer);
                                bzero(buffer,128);
                            }
                            else if(identifier_vec[id]==0){
                                printf("Chave %d: no %d(%s:%s)\n", no.chave_procurada, key_resp, ip_entry, port_entry);//Localização exata do nó procurado
                            }
                            /*
                            bzero(buffer,128);
                            sprintf(buffer,"EPRED %d %s %s\n", key_resp, ip_entry, port_entry); //escrever a mensagem para o nó que estava a espera de entrar no anel---> informaçao do predecessor
                            writemensageudp(fdsucudp,buffer); //escrever a mensagem para o no que quer entrar
                            bzero(buffer,128); //esvaziar o buffer*/
                        }
                    }
                }

                bzero(bufferpred,128); //fazer reset ao buffer

            }

        }

    }

}

