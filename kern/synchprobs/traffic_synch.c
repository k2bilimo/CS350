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
static struct cv *northcv;
static struct cv *southcv;
static struct cv *eastcv;
static struct cv *westcv;
static struct Car* EArray;
static struct Car* WArray;
static struct Car* NArray;
static struct Car* SArray;
static int ECount;
static int WCount;
static int NCount;
static int SCount;
static Direction trafficDirection;
/* 
 * The simulation driver will call this function once before starting
 * the simulation
 *
 * You can use it to initialize synchronization and other variables.
 * 
 */
void
intersection_sync_init(void)
{
  /* replace this default implementation with your own implementation */

  criticalSectionLock= lock_create("intersectionLock");
  if(criticalSectionLock == NULL){
    panic("Could not create intersection Lock");
  }
  northcv = cv_create("North");
  if(northcv == NULL){
  panic("Could not create North Conditional Variable");
  }
  southcv = cv_create("South");
  if(southcv == NULL){
  panic("Could not create South Conditional Variable");
  }
  eastcv= cv_create("East");
  if(eastcv == NULL){
  panic("Could not create East Conditional Variable");
  }
  westcv = cv_create("West");
  if(westcv == NULL){
  panic("Could not create West Conditional Variable");
  }
  return;
  NArray = kmalloc(10* sizeof(struct Car));
  EArray = kmalloc(10* sizeof(struct Car));
  WArray = kmalloc(10* sizeof(struct Car));
  SArray = kmalloc(10* sizeof(struct Car));
  NCount = 0;
  ECount = 0;
  WCount = 0;
  SCount = 0;
  trafficDirection = south;
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
  KASSERT(northcv != NULL);
  KASSERT(southcv != NULL);
  KASSERT(eastcv != NULL);
  KASSERT(westcv != NULL);
  cv_destroy(northcv);
  cv_destroy(southcv);
  cv_destroy(eastcv);
  cv_destroy(westcv);
  lock_destroy(criticalSectionLock);
  kfree(NArray);
  kfree(WArray);
  kfree(EArray);
  kfree(SArray);
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
  KASSERT(northcv != NULL);
  KASSERT(southcv != NULL);
  KASSERT(eastcv != NULL);
  KASSERT(westcv != NULL);
  lock_acquire(criticalSectionLock);
   struct Car currCar;
   currCar.origin = origin;
   currCar.destination = destination;
   if(trafficDirection == south){
   cv_wait(northcv,criticalSectionLock);
   cv_wait(eastcv, criticalSectionLock);
   cv_wait(westcv, criticalSectionLock);
   }
   else if(trafficDirection == north){
   cv_wait(eastcv, criticalSectionLock);
   cv_wait(westcv, criticalSectionLock);
   cv_wait(southcv, criticalSectionLock);
   }
   else if(trafficDirection == east){
   cv_wait(northcv, criticalSectionLock);
   cv_wait(westcv, criticalSectionLock);
   cv_wait(southcv, criticalSectionLock);
   }
   else{
   cv_wait(eastcv, criticalSectionLock);
   cv_wait(northcv, criticalSectionLock);
   cv_wait(southcv, criticalSectionLock);
   }
   if(origin == south) {
   SArray[SCount] = currCar;
   SCount++;
   }
   else if(origin == north){
   NArray[NCount] = currCar;
   NCount++;
   }
   else if(origin == east){
   EArray[ECount] = currCar;
   ECount++;
   }
   else { 
   WArray[WCount] = currCar;
   WCount++;
   }
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
  KASSERT(northcv != NULL);
  KASSERT(southcv != NULL);
  KASSERT(eastcv != NULL);
  KASSERT(westcv != NULL);
  lock_acquire(criticalSectionLock);
	if(trafficDirection == south){
	 cv_signal(westcv,criticalSectionLock);
	 cv_wait(southcv,criticalSectionLock);
	}
	else if(trafficDirection == north){
	cv_signal(eastcv,criticalSectionLock);
	cv_wait(northcv,criticalSectionLock);
	}
	else if(trafficDirection == east){
	cv_signal(southcv, criticalSectionLock);
	cv_wait(eastcv, criticalSectionLock);
	}
	else{
	cv_signal(northcv,criticalSectionLock);
	cv_wait(westcv,criticalSectionLock);
	}
	if(origin == north){
	for(int i = 0; i < NCount; ++i){
	if(NArray[i].origin == origin && NArray[i].destination == destination){
		NArray[NCount] = NArray[i];
		for(int j = i; j < NCount; ++j){
		NArray[j] = NArray[j+1];
		}
		NCount--;
		break;}}}
	else if(origin == south){
	for(int i = 0; i < SCount; ++i){
	if(SArray[i].origin == origin && SArray[i].destination == destination){
		SArray[SCount] = SArray[i];
		for(int j = i; j < SCount; ++j){
		SArray[j] = SArray[j+1];
		}
		SCount--;
		break;}}}
	else if(origin == east){
	for(int i = 0; i < ECount; ++i){
	if(EArray[i].origin == origin && EArray[i].destination == destination){
		EArray[ECount] = EArray[i];
		for(int j = i; j < ECount; ++j){
		EArray[j] = EArray[j+1];
		}
		ECount--;
		break;}}}
	else{
	for(int i = 0; i < WCount; ++i){
	if(WArray[i].origin == origin && WArray[i].destination == destination){
		WArray[WCount] = WArray[i];
		for(int j = i; j < WCount; ++j){
		WArray[j] = WArray[j+1];
		}
		WCount--;
		break;}}}
  lock_release(criticalSectionLock);

}
