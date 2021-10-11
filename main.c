#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

/* 
  Macro para auxiliar no debug e funcionamento do programa.
  Insira -DDEBUG ao compilar para que a macro seja ativada.
*/
#ifdef DEBUG
# define DEBUG_PRINT(x) printf x
#else
# define DEBUG_PRINT
#endif

/* 
  Semáforo que serão usados no programa
  emptySlot: Marca que existe um espaço vazio no buffer, é necessário para o producer.
  wakeConsumer: Marca que um produtor deve acordar, com isso, o produtor trabalha apenas quando necessário.
  lockConsumer: Exclusão mútua para que um consumidor não pegue o mesmo elemento.
  lockWriter: Exclusão mútua para que um escritor escreva por vez.
*/
sem_t emptySlot, wakeConsumer, lockConsumer, lockWriter;

/* 
  buffer: vetor que será usado para guardar os elemetos do vetor.
  threadsIds: Vetor que guarda o ID dos threads.
*/
int *buffer, *threadsIds;

/* 
  threadsNum: Quantidade de threads que será usada pelo programa, o número é o input do usuário + 1.
  blockSize: Tamanho do bloco que deve ser considerado na saída.
  totalElements: Total de elementos na entrada.
  totalCompleted: Total de elementos já processados na saída.
  bufferSize: Quantidade de elementos que será guardado no Buffer, no enunciado foi informado
    que deve ser 10 vezes o tamanho do bloco.
*/
int threadsNum, blockSize, totalElements, totalCompleted = 0, bufferSize;

/* 
  String que guardam o nome dos arquivos que serão usados para leitura e escrita, respectivamente.
*/
char inputFileName[255], outputFileName[255];

/* 
  Threads
*/
pthread_t* threads;

FILE *outputFile;

/* 
  Lê o input do usuário.
  Caso o usuário insira os 4 elementos no ARGV, a entrada não é solicitada.
*/
void readInput(int argc, char const *argv[]) {
  if(argc == 5) { 
    threadsNum = atoi(argv[1]);
    blockSize = atoi(argv[2]);
    strcpy(inputFileName, argv[3]);
    strcpy(outputFileName, argv[4]);
  } else {
    printf("Insira o número de threads: ");
    scanf("%d", &threadsNum);
    printf("Insira o tamanho da entrada: ");
    scanf("%d", &blockSize);
    printf("Insira o nome do arquivo de entrada: ");
    scanf("%s", inputFileName);
    printf("Insira o nome do arquivo de saída: ");
    scanf("%s", outputFileName);
  }

  bufferSize = 10 * blockSize;
}

/* 
  REserva as memórias usando malloc, limpa o arquivo de saída e inicia os semáforos.
*/
void reserveMemory() {
  threads = malloc(sizeof(pthread_t) * threadsNum + 1);
  buffer = malloc(sizeof(int) * bufferSize);
  threadsIds = malloc(sizeof(int) * threadsNum + 1);

  if(threads == NULL || buffer == NULL || threadsIds == NULL) {
    printf("Não foi possível alocar memória\n");
    exit(1);
  }

  fclose(fopen("output.txt", "w"));

  sem_init(&emptySlot, 0, bufferSize);
  sem_init(&wakeConsumer, 0, 0);
  sem_init(&lockConsumer, 0, 1);
  sem_init(&lockWriter, 0, 1);

  outputFile = fopen(outputFileName, "a");
}

/* 
  Limpa a memória e destrói semáforos.
*/
void freeMemory() {
  fclose(outputFile);

  free(threads);
  free(threadsIds);
  free(buffer);

  sem_destroy(&emptySlot);
  sem_destroy(&wakeConsumer);
  sem_destroy(&lockConsumer);
  sem_destroy(&lockWriter);
}

/* 
  Função auxiliar, será usada para ordenar os elementos usando o qsort do stdlib.
*/
int comparator(const void *a, const void *b) {
  return ( *(int*)a - *(int*)b );
}


/* 
  Insere elementos no buffer de uma forma segura.
*/
void insert(int element) {
  static int in = 0;
  static int count = 0;
  sem_wait(&emptySlot);
  buffer[in] = element;
  in = (in + 1) % bufferSize;
  count++;
  // Caso exista um bloco completo, chama um produtor para pegar um bloco.
  if(count == blockSize) {
    sem_post(&wakeConsumer);
    count = 0;
  }
}

/* 
  Remove um bloco do vetor de tamanho blockSize do vetor.
*/
int* pop() {
  int *itens = malloc(sizeof(int) * blockSize);

  if(itens == NULL) {
    printf("Falha ao alocar memoria\n");
    exit(1);
  }

  static int out = 0;
  sem_wait(&lockConsumer);
  for(int i = 0; i < blockSize; i++) {
    itens[i] = buffer[(out+i) % bufferSize];
    sem_post(&emptySlot);
  }
  out = (out + blockSize) % bufferSize;
  sem_post(&lockConsumer);
  return itens;
}

/* 
  Escritor
  Função que será usada para imprimir os elementos no arquivo.
  Assim que um consumidor puxar um bloco, ele irá ordenar e chamar a função para escrever.
  Note que apenas um escritor pode escrever por vez.
*/
void writeBlock(int *vector) {
  sem_wait(&lockWriter);

  if(outputFile == NULL) {
    printf("ERROR: Nao foi possivel abrir o arquivo %s.\n", outputFileName);
    exit(1);
  }

  for(int i = 0; i < blockSize; i++) {
    fprintf(outputFile, "%d ", vector[i]);
  }
  fprintf(outputFile, "\n");


  totalCompleted += blockSize;

  /* 
    Caso todos os elementos tenham sido processados, vamos acordar todos os consumidores.
    Dessa forma eles vão cair na condição e sair de dentro do loop.
  */
  if(totalCompleted == totalElements) {
    for(int i = 0; i < threadsNum+1; i++) {
      sem_post(&wakeConsumer);
    }
  }

  sem_post(&lockWriter);
}

/* 
  Produtor
  Função necessária por ler o arquivos e inserir os elementos no buffer.
*/
void *producer(void *params) {
  FILE *inputFile = fopen(inputFileName, "r");

  if(inputFile == NULL) {
    printf("ERROR: Nao foi possivel abrir o arquivo %s.\n", inputFileName);
    exit(1);
  }

  fscanf(inputFile, "%d", &totalElements);

  for(int i = 0; i < totalElements; i++) {
    int tmp;
    fscanf(inputFile, "%d", &tmp);
    insert(tmp);
  }
  fclose(inputFile);
  pthread_exit(NULL);
}

/* 
  Consumidor
  Primeiro alocamos um vetor para que o consumidor receba um bloco, depois
    esperamos até que um bloco esteja disponível para que o thread receba um bloco.
*/
void *consumer(void *params) {
  int consumerId = *(int*) params;
  DEBUG_PRINT(("C%d - Ligado\n", consumerId));

  int *localBuffer;

  while (1) {
    DEBUG_PRINT(("C%d - Dormindo...\n", consumerId));
    sem_wait(&wakeConsumer);
    DEBUG_PRINT(("C%d - Acordando...\n", consumerId));
    /* 
      Verifica se existe um bloco disponível.
      Caso não exista, o consumidor pode ser encerrado.
    */
    if(totalCompleted == totalElements) break;
    localBuffer = pop();
    qsort(localBuffer, blockSize, sizeof(int), comparator);
    writeBlock(localBuffer);
  }

  DEBUG_PRINT(("C%d - Desligando\n", consumerId));
  pthread_exit(NULL);
}

void creatingThreads() {

  for(int i = 0; i < threadsNum + 1; i++) {
    threadsIds[i] = i;
  }

  // Criando apenas um produtor, com ID = 0, o resto são consumidores.
  pthread_create(&threads[0], NULL, producer, (void *) &threadsIds[0]);
  for(int i = 1; i < threadsNum + 1; i++) {
    pthread_create(&threads[0], NULL, consumer, (void *) &threadsIds[i]);
  }

  // Aguardando todos os threads terminarem.
  for (int i = 0; i < threadsNum + 1; i++) {
    pthread_join(threads[i], NULL);
  }
}

/* 
  A main
*/
int main(int argc, char const *argv[]) {
  readInput(argc, argv);
  reserveMemory();
  creatingThreads();
  freeMemory();
  return 0;
}
