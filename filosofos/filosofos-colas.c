#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
/*
  Autor: Francisco Córdova
*/

//Número de filósofos.
#define N 3

#define PROB_PEN 0.5  // 50% de probabilidad de pensar
#define MAX_PEN 2    // 2 segs. Max tiempo que un filósofo puede pensar
#define MAX_COM 2 // 2 segs. Max tiempo que un filósofo puede emplear comiendo

//Función de espera aleatoria para simular un trabajo distinto para cada proceso.
void random_sleep(size_t t) {
  usleep(rand() % (t*100000)); //Esto espera al azar entre 0 y t segundos.
}

//Arreglos utilizados en el algoritmo. Se inicializan en 0's y tienen tamaño igual a el Número de procesos.
size_t choosing[N] = {};
size_t number[N][N] = {0};
/*
  Función que da el turno para el palillo nt izquierdo.
  Checa todos los turnos que tiene ese palillo y regresa el turno mayor que encontró +1
  Regresa:
    size_t con el turno correspondiente.
*/
size_t max_left(size_t nt, size_t i) {
   size_t max = 0;
   //obtiene el max turno actual en la lista
   for(size_t j = 0; j < N; j++)
      if (max < number[nt][j])
          max = number[nt][j];
   //Para que no ocurra un deadlock.
   /*
      Si ocurre que el filósofo quiere agarrar su palillo izquierdo correspondiente y se da cuenta que
      de momento nadie lo quiere, pero se fija que el filósofo a su izquierda justo va agarrando el
      izquierdo suyo, pues le da chance y se pone como prioridad 3 en el palillo izquierdo
      para que cuando el otro quiera agarrarlo vea que él se puso 3 y entienda que él se puede poner 1 de
      prioridad evitando así el deadlock.
   */
   if (max == 0) //Checa si nadie quiere el palillo (pero en realidad alguien sí lo puede querer así que checamos)
      if (number[(i-1)%N][(i-1)%N] == 1)  //Si su palillo izquierdo ya lo puede agarrar, nos ponemos de prioridad 3 nosotros para ser buena onda.
          max = 2;


   return max;
}
/*
  Parecido al de agarrar el palillo izquierdo pero aquí no nos fijamos en el palillo izquierdo del filósofo anterior sino que checamos la prioridad actual
  de nuestro palillo derecho y si obtenemos un 3, eso quiere decir que el siguiente filósofo nos da chance de agarrarlo para nosotros porque vio que
  ya tenemos nuestro palillo izquierdo básicamente preparado.
*/
size_t max_right(size_t nt, size_t i) {
  size_t max = 0;
  //obtenemos la prioridad de nuestro palillo derecho
  for(size_t j = 0; j < N; j++)
      if (max < number[nt][j])
          max = number[nt][j];
  //Si esta prioridad es 3, eso quiere decir que nos dan chance de agarrarlo así que nos ponemos de prioridad 1 nosotros en ese palillo.
  if (max == 3)
      max = 0;
  return max;
}
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


      /*Esta es la parte de cuando el filósofo desea comer.
        Me basé en el algoritmo empleado en la tarea anterior para hacer esto.
        Al igual que en el algoritmo de la panadería aquí el filósofo tiene que sacar turnos para agarrar un palillo.
        Para evitar el deadlock se utiliza la estrategia planteada anteriormente en las funciones max_left y max_right
      */
      choosing[i] = 1; //True
      number[l][i] = max_left(l,i) + 1;
      number[r][i] = max_right(r,i) + 1;
      choosing[i] = 0; //False
      printf("Al filosofo (%zu) le dio hambre y saco boletos para izq; (%zu):(%zu) y der; (%zu):(%zu) \n",i,l,number[l][i],r,number[r][i]);

      for (size_t j = 0; j < N; j++) {
        while(choosing[j]); //No avanzamos mientras j esté escogiendo

        //No avanzamos mientras el palillo esté ocupado y no seamos el que tiene la mejor prioridad y antiguedad para agarrarlo.
        while(palillo[l] || ((number[l][j] != 0) && ((number[l][j] < number[l][i]) || (( number[l][j] == number[l][i]) && (j < i)))));
        //Igual que arriga pero ahora para el palillo derecho.
        while(palillo[r] || ((number[r][j] != 0) && ((number[r][j] < number[r][i]) || (( number[r][j] == number[r][i]) && (j < i)))));

      }
      //Si la libramos entonces podemos decir que agarramos los palillos...
      palillo[l] = 1;
      printf("El filosofo (%zu) agarra el palillo izq: (%zu)\n",i,l);
      palillo[r] = 1;
      printf("El filosofo (%zu) agarra el palillo der: (%zu)\n",i,r);

      //El filósofo come por un tiempo aleatorio
      //Seccion critica
      printf("El filosofo (%zu) empieza a comer \n", i);
      random_sleep( rand() % MAX_COM + 1);
      printf("El filosofo (%zu) termina de comer \n", i);

      //Cuando deja de comer libera los palillos.
      printf("El filosofo (%zu) deja los palillos (%zu) y (%zu) \n", i,l,r);
      palillo[l] = 0;
      palillo[r] = 0;

      //Ya que el otro filósofo se puede poner 3 en su prioridad, si nosotros intentáramos luego luego sacar otro ticket
      //nos tocaría un turno de 4, pero eso arruinaría la estrategia planteada así que reducimos nuestra prioridad a 0
      //y la de él la ponemos en 1. Para que cuando nosotros saquemos un ticket nos toque un 2, y cuando él ejecute ya que es el único
      //además de nosotros que puede sacar un ticket, nos bajará la prioridad a 1 de nuevo. Y así iremos turnando los palillos mientras
      //estemos peliándonos por él. Si uno de nosotros decide pensar, entonces la cola por el palillo simplemente se vaciará.
      for (size_t idx = 0; idx < N; idx++) {
        if (number[l][idx] == 1) number[l][idx] = 0;
        if (number[l][idx] > 1) number[l][idx] = 1;
        if (number[r][idx] == 1) number[r][idx] = 0;
        if (number[r][idx] > 1) number[r][idx] = 1;
      }

  } while(1); //Comida infinita
}


int main(int argc, char *argv[]) {
  //Se inicializan los N procesos.
  pthread_t procesos[N];

  struct filosofo filosofos[N];

  //Semilla
  srand(time(NULL));

  size_t i;
  //Creamos N hilos, uno por cada filósofo y les pasamos los datos correspondiente de cada filósofo con su hilo.
  for (i = 0; i < N; i++) {
    filosofos[i].id = i;
    filosofos[i].id_palillo_izq = i;
    filosofos[i].id_palillo_der = i+1;
    if (i == N-1)
        filosofos[i].id_palillo_der = 0;

    pthread_create(&procesos[i], NULL, filosofar, &filosofos[i]);
  }

  pthread_exit(NULL);
}
