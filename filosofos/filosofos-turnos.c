#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
/*
  Autor: Francisco Córdova
*/

//Número de filósofos.
#define N 5

#define PROB_PEN 0.5  // 50% de probabilidad de pensar
#define MAX_PEN 2    // 2 segs. Max tiempo que un filósofo puede pensar
#define MAX_COM 2 // 2 segs. Max tiempo que un filósofo puede emplear comiendo

//Función de espera aleatoria para simular un trabajo distinto para cada proceso.
void random_sleep(int t) {
  usleep(rand() % (t*100000)); //Esto espera al azar entre 0 y t segundos.
}

//Arreglos utilizados en el algoritmo. Se inicializan en 0's y tienen tamaño igual a el Número de procesos.
size_t choosing[N] = {};

//Arreglo de palillos donde palillo[2] = 1 denota que el palillo 2 está en uso y palillo[1] = 0 denota que el palillo 1 no está en uso.
size_t palillo[N] = {};

//Estructura de un filósofo usada simplemente para pasar información a los hilos.
struct filosofo {
  size_t id;  //Es el id del filósofo, que va de 0 a N-1
  size_t id_palillo_izq; //Es el id del palillo izquierdo del filósofo, esta variable tomará el valor de id
  size_t id_palillo_der; //Es el id del palillo derecho del filósofo, esta variable tomará el valor de id+1 y si id+1 == NT, entonces el id será 0 para completar el ciclo.
};

/*
  Función principal que denota lo que hará cada filósofo.
  Cada hilo correrá una copia de esta función concurrentemente con los demás hilos.
*/
void *filosofar(void *args){
  //Obtenemos el id del filósofo (i), y los id's correspondientes a sus palillos (l[de left] y r[de right])
  struct filosofo *f = (struct filosofo*) args;
  size_t i = f->id;
  size_t l = f->id_palillo_izq;
  size_t r = f->id_palillo_der;
  //Ahora que sabemos todo lo necesario del filósofo procedemos al algoritmo...
  do {
      /*Es bien conocido que el filósofo tiene 2 acciones, pensar o comer. Para decidir cuál quiere hacer se tira un random
      con la probabilidad que tienen los filósofos para pensar (PROB_PEN).
      Si el filósofo decide comer, entonces el ciclo se romperá e irá a las siguientes instrucciones.
      Si el filósofo decide pensar, entrará al ciclo while donde pensará (con un sleep random) un tiempo aleatorio entre 0 y MAX_PEN segundos.
      Esta configuración también permite al filósofo decidir volver a pensar después de haber pensado. De tal manera que puede pasar todo el tiempo
      pensando que quiera hasta que decida comer.
      */
      while( (rand() % 10) > PROB_PEN*10 ) {
        printf("El filosofo (%zu) decide pensar...\n", i);
        random_sleep( rand() % MAX_PEN + 1 );
      }

      //El filósofo se cansa de pensar y decide comer un rato...
      printf("Al filosofo (%zu) le dio hambre y solicita comer...\n",i);
      choosing[i] = 1;
      printf("El filosofo (%zu) espera su turno...\n",i);
      while (choosing[i]); //Espera a que el monitor le de permiso de comer.
      printf("El filosofo (%zu) espera que le den su palillo izquierdo (%zu)...\n",i,l);
      while (!palillo[l]); //Espera a que el monitor le de el palillo izquierdo
      printf("El filosofo (%zu) espera que le den su palillo derecho (%zu)...\n",i,r);
      while (!palillo[r]); //Espera a que el monitor le de el palillo derecho.
      printf("El filosofo (%zu) empieza a comer...\n",i);
      random_sleep( rand() % MAX_COM + 1 );  //Come en un tiempo random.
      printf("El filosofo (%zu) termina de comer...\n",i);
      palillo[l] = 0; //Deja el palillo izq
      printf("El filosofo (%zu) regresa el palillo izquierdo (%zu)...\n",i,l);
      palillo[r] = 0; //Deja el palillo derecho.
      printf("El filosofo (%zu) regresa el palillo izquierdo (%zu)...\n",i,r);

  } while(1); //Comida infinita
}
/*
  Esta función es ejecutada por un hilo monitor, el cual se encarga de cambiar de turno y asignarle los paillos a aquel que decida comer.
*/
void *monitorear(void *args) {
  //La variable TURNO representa el turno actual en la mesa.
  int TURNO = 1;
  do {
    //Si el turno es 1, esto lo cambiará a 0 y si es 0 lo cambiará a 1.
    TURNO = 1 - TURNO;
    /*
      Empezamos viendo al filósofo (0 ó 1) dependiendo del turno y nos movemos de 2 en 2.
      De tal manera que si el turno es 0, el orden sería 0,2,4,6,8,... N-2
      Y si fuera 1, sería 1,3,5,7,9,...N-2

      La razón por la cual no se checa al último filósofo (por eso es N-2 y no N-1) es porque si el número de filósofos es impar
      por ejemplo 5:  [0,1,2,3,4]. El filósofo 0 y el filósofo 4 tendrían el mismo turno, y en teoría deberían poder comer pero ellos son los únicos
      con el mismo turno que comparten un palillo. Por lo que el caso del último filósofo se checa por separado.
    */
    for (size_t i = TURNO; i < N-1; i+=2)  //Checamos a todos los filósofos menos al último que estén en su turno.
      if (choosing[i]) {  //Si éste quiere comer, entonces procedemos
        choosing[i] = 0;  //Le decimos que puede comer.
        palillo[i] = 1;  // Le damos el palillo izquierdo
        palillo[(i+1) % N] = 1; //Le damos el palillo derecho.
      }

    if (choosing[N-1] && ((N-1) % 2 == TURNO)) {  //Si el último filósofo quiere comer y está en su turno procedemos
      while (palillo[N-1]); //Este no hace pero por simetría se lo dejo.
      while (palillo[0]); //Si este palillo está siendo utilizado quiere decir que el filósofo inicial tiene el mismo turno y está comiendo por lo que no podemos darle el palill al último filósofo.
      choosing[N-1] = 0; //Cuando se libera el palillo le decimos al filósofo final que puede comer.
      palillo[N-1] = 1; //Le damos el palillo izquierdo.
      palillo[0] = 1; //Y le damos el palillo derecho que sabemos que ya lo liberó el primer filósofo.
    }

    for (size_t i = 0; i < N; i++) //Esperamos a que todos los palillos dejen de ser utilizados para cambiar de turno.
      while(palillo[i]);

  } while(1);
}


int main(int argc, char *argv[]) {
  //Se inicializan los N procesos.
  pthread_t procesos[N];
  pthread_t monitor;

  struct filosofo filosofos[N];

  //Semilla
  srand(time(NULL));

  size_t i;
  //Creamos N hilos, uno por cada filósofo y les pasamos los datos correspondiente de cada filósofo con su hilo.
  for (i = 0; i < N; i++) {
    filosofos[i].id = i;
    filosofos[i].id_palillo_izq = i;
    filosofos[i].id_palillo_der = i+1;
    if (i == N-1) filosofos[i].id_palillo_der = 0;

    pthread_create(&procesos[i], NULL, filosofar, &filosofos[i]);
  }

  pthread_create(&monitor, NULL, monitorear, NULL);


  pthread_exit(NULL);
}
