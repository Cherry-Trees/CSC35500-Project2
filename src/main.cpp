/**
   Author: Jamie Miles
   Date: 11/4/24
 */
#include "AirportAnimator.hpp"

#include <unistd.h>
#include <iostream>
#include <pthread.h>
#include <semaphore.h>

#define NUM_PLANES 8
#define PLANE_PASSENGER_CAPACITY 10

int airport_passengers = 0;
int scheduled_tours = 0;
int completed_tours = 0;
int in_process = 0;

// Boarding and runway semaphores.
sem_t board_sem, runway_sem;

void *airplane_process(void *pthread_id) {
  int plane_id = *(int *)pthread_id;

  // How many threads are currently running.
  ++in_process;

  // Minus 1 to account for this thread.
  while (completed_tours + in_process - 1 < scheduled_tours) {

    // Board passengers.
    int boarded_passengers = 0;
    AirportAnimator::updateStatus(plane_id, "BOARD");
    while (boarded_passengers < PLANE_PASSENGER_CAPACITY) {
      sem_wait(&board_sem);

      // Only board a passenger if there's one available in the airport.
      if (airport_passengers > 0) {
	++boarded_passengers;
	--airport_passengers;
      }
      AirportAnimator::updatePassengers(plane_id, boarded_passengers);
      sem_post(&board_sem);
      sleep(rand() % 3);
    }

    // Taxi out.
    AirportAnimator::updateStatus(plane_id, "TAXI");
    AirportAnimator::taxiOut(plane_id);
    
    // Tour.
    sem_wait(&runway_sem);
    AirportAnimator::updateStatus(plane_id, "TKOFF");
    AirportAnimator::takeoff(plane_id);
    sem_post(&runway_sem);

    // Tour.
    AirportAnimator::updateStatus(plane_id, "TOUR");
    sleep(5 + rand() % 41);

    // Land.
    sem_wait(&runway_sem);
    AirportAnimator::updateStatus(plane_id, "LAND");
    AirportAnimator::land(plane_id);
    sem_post(&runway_sem);

    // Taxi in.
    AirportAnimator::updateStatus(plane_id, "TAXI");
    AirportAnimator::taxiIn(plane_id);

    // Release passengers.
    AirportAnimator::updateStatus(plane_id, "DEPLN");
    while (boarded_passengers > 0) {
      sem_wait(&board_sem);
      --boarded_passengers;
      ++airport_passengers;
      AirportAnimator::updatePassengers(plane_id, boarded_passengers);
      sem_post(&board_sem);
      sleep(1);
    }

    ++completed_tours;
    AirportAnimator::updateTours(completed_tours);
  }
  --in_process;
  AirportAnimator::updateStatus(plane_id, "DONE");
  return pthread_id;
}

int main(int argc, char *argv[]) {
  
  if (argc != 3) {
    std::cerr << "Usage: <Passengers> <Tours>" << std::endl;
    return 1;
  }
  
  airport_passengers = std::atoi(argv[1]);
  scheduled_tours = std::atoi(argv[2]);

  // Initialize semaphores with a starting value of 1.
  sem_init(&board_sem, 0, 1);
  sem_init(&runway_sem, 0, 1);

  AirportAnimator::init();

  // Create the 8 threads.
  pthread_t planes[NUM_PLANES];
  int plane_ids[NUM_PLANES];
  for (int i = 0; i < NUM_PLANES; i++) {
    plane_ids[i] = i;
    pthread_create(planes + i, NULL, airplane_process, (void *)(plane_ids + i));
  }

  // Join the threads after they are done running.
  for (pthread_t &plane : planes)
    pthread_join(plane, NULL);

  AirportAnimator::end();

  // Clean semaphores.
  sem_destroy(&board_sem);
  sem_destroy(&runway_sem);
  
  return 0;
}
