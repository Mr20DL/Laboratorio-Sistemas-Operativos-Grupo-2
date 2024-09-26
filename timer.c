#include "devices/timer.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include <list.h>

/* Estructura que guarda un hilo y su tiempo de despertar */
struct sleep_thread {
  struct thread *thread;    // El hilo que está dormido
  int64_t wake_up_time;     // El tiempo en ticks cuando debe despertar
  struct list_elem elem;    // Elemento para la lista de hilos dormidos
};

static struct list sleeping_list; // Lista de hilos dormidos

/* Inicializa la lista de hilos dormidos */
void timer_init(void) {
  list_init(&sleeping_list);
}

/* Función auxiliar para ordenar hilos por su tiempo de despertar */
bool sleep_less_func(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED) {
  struct sleep_thread *sa = list_entry(a, struct sleep_thread, elem);
  struct sleep_thread *sb = list_entry(b, struct sleep_thread, elem);
  return sa->wake_up_time < sb->wake_up_time;
}

/* Función modificada de timer_sleep */
void timer_sleep(int64_t ticks) {
  if (ticks <= 0) return;

  int64_t start = timer_ticks();
  enum intr_level old_level = intr_disable();  // Desactiva interrupciones

  // Crear un objeto para el hilo que quiere dormir
  struct sleep_thread st;
  st.thread = thread_current();
  st.wake_up_time = start + ticks;

  // Insertar el hilo en la lista ordenada por el tiempo de despertar
  list_insert_ordered(&sleeping_list, &st.elem, sleep_less_func, NULL);

  // Bloquear el hilo hasta que se despierte
  thread_block();
  
  intr_set_level(old_level);  // Reactivar interrupciones
}

/* Manejador de interrupciones del temporizador */
void timer_interrupt(struct intr_frame *args UNUSED) {
  int64_t current_ticks = timer_ticks();
  struct list_elem *e = list_begin(&sleeping_list);

  // Despertar hilos cuyo tiempo de despertar ha llegado
  while (e != list_end(&sleeping_list)) {
    struct sleep_thread *st = list_entry(e, struct sleep_thread, elem);
    if (st->wake_up_time > current_ticks) {
      break;
    }
    e = list_remove(e);  // Remover el hilo de la lista
    thread_unblock(st->thread);  // Desbloquear el hilo
  }

  // Otras tareas de la interrupción de temporizador
  thread_tick();
}
