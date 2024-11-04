/**
   Author: Jamie Miles
   Date: 11/4/24
 */
#include "AirportAnimator.hpp"

#include <unistd.h>
#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <algorithm>

#define NUM_PLANES 8
#define PLANE_PASSENGER_CAPACITY 10

int airport_passengers = 0;
int scheduled_tours = 0;

// Completed trips per plane.
int completed_trips[NUM_PLANES];

// Boarding and runway semaphores.
sem_t board_sem, runway_sem;

void *airplane_process(void *pthread_id) {
  int plane_id = *(int *)pthread_id;
  completed_trips[plane_id] = 0;

  while (completed_trips[plane_id] < scheduled_tours) {

    // Board passengers.
    int boarded_passengers = 0;
    while (boarded_passengers < PLANE_PASSENGER_CAPACITY && airport_passengers > 0) {
      sem_wait(&board_sem);
      ++boarded_passengers;
      --airport_passengers;
      AirportAnimator::updatePassengers(plane_id, boarded_passengers);
      sem_post(&board_sem);
      sleep(rand() % 3);
    }

    // Taxi out.
    AirportAnimator::updateStatus(plane_id, "TAXI");
    AirportAnimator::taxiOut(plane_id);
    
    // Take off.
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
    while (boarded_passengers > 0) {
      sem_wait(&board_sem);
      --boarded_passengers;
      ++airport_passengers;
      AirportAnimator::updatePassengers(plane_id, boarded_passengers);
      sem_post(&board_sem);
      sleep(rand() % 3);
    }

    // Increment this plane's completed trips by 1.
    // Update completed tours to be the minimum of all planes' completed trips.
    ++completed_trips[plane_id];
    AirportAnimator::updateTours(*std::min_element(completed_trips, completed_trips + NUM_PLANES));
  }
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
