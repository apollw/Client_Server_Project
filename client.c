/*Um servidor simples no domínio da internet usando TCP
  O número da porta é passado como argumento */

//Esse arquivo de cabeçalho contém declarações usadas na maioria dos input
//e output e é tipicamente incluso em todos os programas C
#include <stdio.h>
//Esse arquivo de cabeçalho define quatro tipos de variável, alguns macros
//e várias funções para performar funçõẽs gerais.Ex.: int atoi(const char *str):
//Converte a string apontada, pelo argumento str  para um inteiro  (type int).
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
//Esse arquivo de cabeçalho contém definições de um número de tipos de dados
//usados em syscalls
#include <sys/types.h>
//Esse arquivo de cabeçalho inclui um número de definições de estruturas
//necessárias para sockets. Ex.: Define o sockaddr structure
#include <sys/socket.h>
//Esse arquivo de cabeçalho contém constantes e estruturas necessárias para
// endereços de domínio na internet. Ex.: sockaddr_in
#include<netinet/in.h>
#include<netdb.h>

void error(const char *msg){
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {

    //int sockfd é a nossa variável. Ela possui 3 argumentos:
    //int sockfd = socket(int domain, int type, int protocol)
    //Os 3 argumentos especificam o protocolo:
    //O primeiro argumento especifica o domínio da comunicação:em nosso
    //caso usamos a família AF_INET, que especifica IPv4
    //O segundo argumento define dois tipos.Estaremos usando SOCK_STREAM,
    //que especifica o protocolo TCP
    //No terceiro argumento, 0 é o padrão para TCP

    // sockfd → descritor do socket.
    // É um "arquivo especial" criado pelo kernel que representa
    // uma conexão de rede (TCP nesse caso).
    // Tudo que enviarmos/recebermos será feito usando esse número.
    int sockfd, portno, n;

    // Estrutura que representa um endereço IPv4.
    // Aqui vamos armazenar:
    // - família (AF_INET → IPv4)
    // - endereço IP do servidor
    // - porta do servidor
    // Isso pertence à camada de REDE (IP).
    struct sockaddr_in serv_addr;

    // Estrutura usada para armazenar o resultado da resolução DNS.
    // gethostbyname() transforma "google.com" → endereço IP.
    // Ou seja, converte nome (camada aplicação) para IP (camada rede).
    struct hostent *server;

    // Buffer onde armazenamos dados enviados e recebidos.
    // Aqui trafega o payload da aplicação (texto, HTTP, etc).
    char buffer[256];

    // argc → quantidade de argumentos passados na linha de comando.
    // argv → vetor contendo esses argumentos.
    //
    // Esperamos:
    // argv[0] → nome do programa
    // argv[1] → hostname (ex: google.com)
    // argv[2] → porta (ex: 80)
    //
    // Se não forem passados pelo menos 3 argumentos,
    // não temos informação suficiente para conectar.
    if(argc<3) {

        // Mostra como o programa deve ser executado.
        // %s imprime argv[0], que é o nome do executável.
        fprintf(stderr,"usage %s hostname port\n",argv[0]);

        //Encerra o programa.
        exit(0);
    }

    // Converte a string da porta (argv[2]) para inteiro.
    // Ainda estamos na camada de aplicação (processando entrada do usuário).
    portno = atoi(argv[2]);

    // Cria um socket no kernel.
    // AF_INET  → camada de rede (IPv4)
    // SOCK_STREAM → camada de transporte (TCP)
    // 0 → protocolo padrão para TCP
    //
    // Aqui estamos pedindo ao kernel:
    // "Quero um endpoint TCP sobre IPv4."
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // Se retornar < 0, o kernel falhou ao criar o socket.
    if(sockfd<0)
        error("ERROR opening socket");

    // Resolve o hostname (ex: google.com) para um endereço IP.
    // Isso envolve DNS.
    // Estamos traduzindo um nome (aplicação) para IP (camada de rede).
    server = gethostbyname(argv[1]);

    // Se for NULL, o DNS falhou.
    if(server==NULL){
        fprintf(stderr,"ERROR: no such host\n");
        exit(0);
    }

    // Zera a estrutura de endereço.
    // Boa prática para evitar lixo de memória.
    bzero((char*)&serv_addr, sizeof(serv_addr));

    // Define a família como IPv4.
    serv_addr.sin_family  = AF_INET;

    // Copia o IP resolvido pelo DNS para a estrutura do endereço.
    // Agora serv_addr contém o IP do servidor.
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);

    // Converte a porta para "network byte order".
    //
    // htons = host to network short
    //
    // CPUs podem ser little-endian.
    // A rede usa big-endian.
    // Isso garante que os bytes trafeguem no padrão correto.
    serv_addr.sin_port = htons(portno);

    /*Aqui acontece:
    --> Kernel envia SYN
    --> Servidor responde SYN-ACK
    --> Kernel envia ACK
    Se isso falhar → retorna erro.
    Não implementamos o handshake. O kernel implementa.
    O programa só solicita.*/
    if(connect(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0)
        error("ERROR connecting");

    printf("Client: ");

    while(1){
        //Limpa o buffer
        bzero(buffer,256);

        //Lê texto digitado no terminal
        fgets(buffer,255,stdin);

        //Envia os bytes pelo socket TCP.
        //TCP vai fragmentar, numerar e garantir entrega
        n = write(sockfd, buffer, strlen(buffer));

        if(n<0)
            error("ERROR writing to socket");

        bzero(buffer,256);

        // Lê resposta do servidor.
        // read() bloqueia até chegar dados.
        n = read(sockfd, buffer, 255);

        if(n<0)
            error("ERROR reading from socket");

        printf("Server: %s\n", buffer);
        // Se servidor enviar algo começando com "Bye",
        // encerramos a conexão.
        int i = strncmp("Bye", buffer, 3);

        if(i==0)
            break;
    }

    // Fecha o socket.
    // TCP envia FIN e encerra conexão corretamente.
    close(sockfd);

    /*Quando chamamos close():
    --> Kernel envia FIN
    --> Servidor responde ACK
    --> Servidor envia FIN
    --> Kernel responde ACK
    Encerramento elegante da conexão TCP.*/

}
