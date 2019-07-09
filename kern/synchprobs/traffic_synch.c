#include <types.h>
#include <lib.h>
#include <synchprobs.h>
#include <synch.h>
#include <opt-A1.h>
#include <array.h>
/* 
 * This simple default synchronization mechanism allows only vehicle at a time
 * into the intersection.   The intersectionSem is used as a a lock.
 * We use a semaphore rather than a lock so that this code will work even
 * before locks are implemented.
 */

/* 
 * Replace this default synchronization mechanism with your own (better) mechanism
 * needed for your solution.   Your mechanism may use any of the available synchronzation
 * primitives, e.g., semaphores, locks, condition variables.   You are also free to 
 * declare other global variables if your solution requires them.
 */

/*
 * replace this with declarations of any synchronization and other variables you need here
 */
struct Car{
Direction origin;
Direction destination;
};

static struct lock *criticalSectionLock;
static struct cv *buffer;
static struct Car *carArray;
static int carCount;
bool right_turn(struct Car car);
bool right_turn_condition(struct Car carA, struct Car carB);
bool same_direction(struct Car carA, struct Car carB);
bool opposite_direction(struct Car carA, struct Car carB);
bool not_safe_to_enter(struct Car car);

/* 
 * The simulation driver will call this function once before starting
 * the simulation
 *
 * You can use it to initialize synchronization and other variables.
 * 
 */
bool right_turn(struct Car car){
if((car.origin == north && car.destination == west) || 
(car.origin == south && car.destination == east) || 
(car.origin == west && car.destination == south) ||
(car.origin == east && car.destination == north)){
return true;
}
return false;
}
bool right_turn_condition(struct Car carA, struct Car carB){
if((right_turn(carA) || right_turn(carB)) && carA.destination != carB.destination) return true;
return false;
}
bool same_direction(struct Car carA, struct Car carB){
if(carA.origin == carB.origin)return true;
return false;
}
bool opposite_direction(struct Car carA, struct Car carB){
if(carA.origin == carB.destination && carA.destination == carB.origin) return true;
return false;
}
bool not_safe_to_enter(struct Car car){
for(int i = 0; i < carCount; ++i){
if(!same_direction(car, carArray[i]) && 
!opposite_direction(car,carArray[i]) &&
!right_turn_condition(car,carArray[i])) return true;
}
return false;
}
void
intersection_sync_init(void)
{
  /* replace this default implementation with your own implementation */

  criticalSectionLock= lock_create("intersectionLock");
  if(criticalSectionLock == NULL){
    panic("Could not create intersection Lock");
  }
  buffer = cv_create("Buffer");
  if(buffer == NULL){
  panic("Could not create Buffer Conditional Variable");
  }
  carArray = kmalloc(sizeof(struct Car));
  carCount = 0;
}


/* 
 * The simulation driver will call this function once after
 * the simulation has finished
 *
 * You can use it to clean up any synchronization and other variables.
 *
 */
void
intersection_sync_cleanup(void)
{
  /* replace this default implementation with your own implementation */
  KASSERT(criticalSectionLock != NULL);
  KASSERT(buffer != NULL);
  cv_destroy(buffer);
  lock_destroy(criticalSectionLock);
  kfree(carArray);
}


/*
 * The simulation driver will call this function each time a vehicle
 * tries to enter the intersection, before it enters.
 * This function should cause the calling simulation thread 
 * to block until it is OK for the vehicle to enter the intersection.
 *
 * parameters:
 *    * origin: the Direction from which the vehicle is arriving
 *    * destination: the Direction in which the vehicle is trying to go
 *
 * return value: none
 */

void
intersection_before_entry(Direction origin, Direction destination) 
{
  /* replace this default implementation with your own implementation */
  KASSERT(buffer != NULL);
  lock_acquire(criticalSectionLock);
  // write Car. 
   struct Car currCar;
   currCar.origin = origin;
   currCar.destination = destination;
   while(not_safe_to_enter(currCar)){
   cv_wait(buffer, criticalSectionLock);
   }
   carArray[carCount] = currCar; 
   ++carCount;
   lock_release(criticalSectionLock);
}


/*
 * The simulation driver will call this function each time a vehicle
 * leaves the intersection.
 *
 * parameters:
 *    * origin: the Direction from which the vehicle arrived
 *    * destination: the Direction in which the vehicle is going
 *
 * return value: none
 */

void
intersection_after_exit(Direction origin, Direction destination) 
{
  KASSERT(buffer != NULL);
  lock_acquire(criticalSectionLock);
  for(int i = 0; i < carCount; ++i){
  if(carArray[i].origin == origin && carArray[i].destination == destination){
	for(int j = i; j < carCount-1; ++j){
	carArray[j] = carArray[j+1];
	}
	--carCount;
	break;
  }
  }
  cv_signal(buffer,criticalSectionLock);
  lock_release(criticalSectionLock);
}
