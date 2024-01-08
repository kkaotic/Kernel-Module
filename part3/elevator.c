#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/delay.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kaleb Szaro, Jacob Herren, John McLeod");
MODULE_DESCRIPTION("A not-so-simple Linux Kernel Module that moves passengers through an elevator from floor to floor.");
MODULE_VERSION("0.1");

#define ENTRY_NAME "elevator"
#define PERMS 0666
#define PARENT NULL

static struct proc_dir_entry* proc_entry; //pointer to proc entry
int proc_len; //num of bytes for proc to copy to user//

// main Elevator logic function prototype
int Elevator(void*);

int start_elevator(void);                                                           // starts the elevator to pick up and drop off passengers
int issue_request(int start_floor, int destination_floor, int type);                // add passengers requests to specific floors
int stop_elevator(void);                                                            // stops the elevator

extern int (*STUB_start_elevator)(void);
extern int (*STUB_issue_request)(int,int,int);
extern int (*STUB_stop_elevator)(void);


//declare elevator struct containing all
//elevator values and list
struct {
        int floor;
	int serviced;
        int wcapacity;
        int pcapacity;
        char * state;
	struct list_head list;
} elevator;

// declare array of floor linked lists
struct {
	struct list_head list;
} floors[6];

//Person struct
typedef struct person {
        int dest;
        int start;
        int type;
        struct list_head next;
} Person;


bool stopped;		//lets the elevator know to start shutting down
bool requesting;	//used for locks
bool exitout;		//signals to stop the Elvator loop

//Kthread to run the elevator
static struct task_struct * kthread_elevator;

//create mutex
struct mutex mutex;

int start_elevator(void) {
	exitout = false;
	stopped = false;
	// init kthread
	elevator.state = "IDLE";
        kthread_elevator = kthread_run(Elevator, NULL, "Elevator_thread");
        return 0;
}

int issue_request(int start_floor, int destination_floor, int type) {

	while(1){
		if(mutex_lock_interruptible(&mutex) == 0 ){
			// Initialize array of floor linked lists
        		// MAKE SURE THIS DOESNT GET CALLED EVERY TIME ISSUE_REQUEST IS CALLED
        		// will delete already existing data

        		// make a person and add to starting floor
        		Person * person;
        		person = kmalloc(sizeof(Person), __GFP_RECLAIM);
        		person->start = start_floor;
        		person->dest = destination_floor;
        		person->type = type;
        		list_add_tail(&person->next, &floors[start_floor-1].list);
        		requesting = false;
       			mutex_unlock(&mutex);
			break;
		}
	}
        return 0;
}


int stop_elevator(void) {
	kthread_stop(kthread_elevator);
	return 0;
}

//__________________________
//ELEVATOR LOGIC FUNCTIONS//
//__________________________

//prototype for function that scans for populated floors
//when the elevator's current floor is empty
int Seeker(int floor);

//main logic for moving the elevator. Functions based on a
//set of bool values and the contents of the 6 floors
//and the contents of the elevator's list
void Move(void) {
        //Logic for determining target floor
        int targetFloor = 0;
        Person *first;
	//runs if there are passengers on the elevator
        if(!(list_empty(&elevator.list))) {
                first = list_first_entry(&elevator.list, Person, next);
                targetFloor = first->dest;
        }
	//runs if no passengers on the elevator
        else {
                if(stopped) {
                        elevator.state = "OFFLINE";
			exitout = true;
                        return;
                }
                targetFloor = Seeker(elevator.floor);
        }
	//0 gets returned from seeker if there are no students
	//waiting on any floor, state is set to IDLE
        if(targetFloor == 0) {
                elevator.state = "IDLE";
                return;
        }

        //move logic
        if(targetFloor < elevator.floor) {
                elevator.state = "DOWN";
                while(targetFloor < elevator.floor) {
                        elevator.floor -= 1;
			ssleep(2);
                }
        }
        if(targetFloor > elevator.floor) {
                elevator.state = "UP";
                while(targetFloor > elevator.floor) {
                        elevator.floor += 1;
			ssleep(2);
                }
        }
}


//function that handles offloading students in the elevator at their
//destination floor when the Move() function moves the elevator to
//the correct floor
void Offload(void) {
	//Set state to LOADING only when there are passengers on
	//the elevator
	if(!list_empty(&elevator.list))
        	elevator.state = "LOADING";
	//array to make retreiving student weights easy
	int weight[4] = {100, 150, 200, 250};
        //Offload logic
        //walkthru list and check for passengers to offload
        struct list_head *temp;
        struct list_head *dummy;
        Person * person;

        list_for_each_safe(temp, dummy, &elevator.list)  {
                person = list_entry(temp, Person, next);
                if(person->dest == elevator.floor) {
			elevator.wcapacity -= weight[person->type];
			elevator.pcapacity -= 1;
                        list_del(temp);
                        kfree(person);
			elevator.serviced++;
                }
        }
}

//function to handle loading students from the elevator's current floor
void Load(void) {
	//if stopped, the elevator will not be loaded
	if(stopped)
		return;
	while(1){
		if(mutex_lock_interruptible(&mutex) == 0) {
			//array for making retreiving student weights easy
        		int weight[4]= {100, 150, 200, 250};
			//sets state to LOADING if there are students waiting on
			//the current floor
			if(!(list_empty(&floors[elevator.floor].list)))
				elevator.state = "LOADING";
        		//Load logic
       			int slot = elevator.floor - 1;
        		struct list_head *temp;
        		struct list_head *dummy;
        		Person *person;

			//Walk through list and add the first student to the elevator
			//If a student in front does not fit, it will check the rest of
			//the list person by person to see if anyone else will fit
			//If the elevator hits capacity, the loop breaks and the
			//the elevator continues moving
        		list_for_each_safe(temp, dummy, &floors[slot].list) {
        			//if(requesting)
				person = list_entry(temp, Person, next);
                		if(((weight[person->type] + elevator.wcapacity)
                		<= 750) && (elevator.pcapacity < 5)) {
                        		list_move_tail(&person->next, &elevator.list);
                        		elevator.wcapacity += weight[person->type];
                        		elevator.pcapacity += 1;
                		}
               			if(elevator.wcapacity == 750 || elevator.pcapacity == 5)
                        		break;
			}
			ssleep(1);
			mutex_unlock(&mutex);
                        break;
		}
        }
}

//Function that handles scanning for floors that have waiting
//students when the elevator is empty
//Takes in the current floor and checks if it has students,
//then checks the floor below and above, then checks the floors
//below and above those, limited at floors 1 and 6
//Returns the floor number of the nearest floor with waiting
//students with a lower floor bias
int Seeker(int floor) {
	if(!(list_empty(&floors[floor-1].list)))
		return floor;
        int lowerSearch = floor;
        int upperSearch = floor;
        for(int i = 0; i < 6; i++) {
                if(lowerSearch - 1 >= 1) {
                        lowerSearch -= 1;
                        if(!(list_empty(&floors[lowerSearch-1].list))) {
                                return lowerSearch;
                        }
                }
                if(upperSearch + 1 <= 6) {
                        upperSearch += 1;
                        if(!(list_empty(&floors[upperSearch-1].list))) {
                                return upperSearch;
                        }
                }
        }
        return 0;
}

//Main elevator loop
//This is the function that is passed to the elevator kthread
int Elevator(void *data) {
        while(!exitout) {
		stopped = false;
		if(kthread_should_stop())
			stopped = true;
                Move();
                Offload();
                Load();
        }
	return 0;
}


//________________________
//ELEVATOR LOGIC END//
//________________________


//PROC SECTION FOR ELEVATOR DISPLAY//

static ssize_t elevator_read(struct file* file, char* ubuf, size_t count, loff_t *ppos)
{
	char *buffer = kmalloc(sizeof(char) * 10000, __GFP_RECLAIM);
	if (buffer == NULL) {
		printk(KERN_WARNING "elevator_read");
		return -ENOMEM;
	}
	proc_len = 0;

	int floor_count = 0;
	int floor;


	proc_len = sprintf(buffer, "Elevator state: %s\n", 
	elevator.state); //call state c-string, print it to state

	proc_len += sprintf(buffer + proc_len, "Current Floor: %d\n", 
	elevator.floor);

	proc_len += sprintf(buffer + proc_len, "Current Load: %d\n", 
	elevator.wcapacity);  //add weight of each student obj in elev.list

	proc_len += sprintf(buffer + proc_len, "Elevator Status: ");

	struct list_head *it;
	if (!list_empty(&elevator.list))
	{
		Person *person;
		list_for_each(it, &elevator.list){
			person = list_entry(it, Person, next);
                        switch(person->type) {
	                        case 0:
                                	proc_len += sprintf(buffer + 
					proc_len, "F");
                                        break;
                                case 1:
               		                proc_len += sprintf(buffer + 
					proc_len, "O");
                                        break;
                                case 2:
                                        proc_len += sprintf(buffer + 
					proc_len, "J");
                                        break;
                                case 3:
                                        proc_len += sprintf(buffer + 
					proc_len, "S");
                                        break;
                        }
			proc_len += sprintf(buffer + proc_len, "%d ", 
			person->dest);
		}
	}
	proc_len += sprintf(buffer + proc_len, "\n\n");  
	//print Class & End Floor of each list object 

	//PRINT EACH FLOOR AND CORRESPONDING PERSONS ON FLOOR//
	for (int i = 6; i > 0; i--)
	{
		floor_count = 0;
		floor = i-1;
		if(elevator.floor == i)
		{
			proc_len += sprintf(buffer + proc_len, 
			"[*] Floor %d: ", elevator.floor);

			if (list_empty(&floors[floor].list)){
                		proc_len += sprintf(buffer + proc_len, 
				"%d ", floor_count);
        		}
			else
        		{
				struct list_head *f_it;
				struct list_head *fl_it;
				Person * person;
		                list_for_each(fl_it, &floors[floor].list)
		                {
                		        floor_count++;
                		}
                		proc_len += sprintf(buffer + proc_len, 
				"%d ", floor_count);

				list_for_each(f_it, &floors[floor].list)
                		{
                        		person = list_entry(f_it, Person, 
					next);
                        		switch(person->type) {
                                		case 0:
                                        		proc_len += sprintf
							(buffer + proc_len, 
							"F");
                                        		break;
                                		case 1:
                                        		proc_len += sprintf
							(buffer + proc_len, 
							"O");
                                        		break;
		                                case 2:
                		                        proc_len += sprintf
							(buffer + proc_len, 
							"J");
                                		        break;
		                                case 3:
                		                        proc_len += sprintf
							(buffer + proc_len, 
							"S");
                                		        break;
                        		}
					proc_len += sprintf(buffer + 
					proc_len, "%d ", person->dest);
				}
			}
		}
		else
		{
			struct list_head *f_it;
			struct list_head *fl_it;
			proc_len += sprintf(buffer + proc_len, 
			"[ ] Floor %d: ", i);

			if (list_empty(&floors[floor].list)){
                                proc_len += sprintf(buffer + proc_len, 
				"%d ", floor_count);
                        }
                        else
                        {
				Person * person;
                                list_for_each(fl_it, &floors[floor].list)
                                {
                                        floor_count++;
                                }
                                proc_len += sprintf(buffer + proc_len, 
				"%d ", floor_count);

                                list_for_each(f_it, &floors[floor].list)
                                {
                                        person = list_entry(f_it, Person, next);
                                        switch(person->type) {
                                                case 0:
                                                        proc_len += sprintf
							(buffer + proc_len, 
							"F");
                                                        break;
                                                case 1:
                                                        proc_len += sprintf
							(buffer + proc_len, 
							"O");
                                                        break;
                                                case 2:
                                                        proc_len += sprintf
							(buffer + proc_len, 
							"J");
                                                        break;
                                                case 3:
                                                        proc_len += sprintf
							(buffer + proc_len, 
							"S");
							break;
					}
					proc_len += sprintf(buffer + 
					proc_len, "%d ", person->dest);
				}
			}
		}
		proc_len += sprintf(buffer + proc_len, "\n");

	}
	proc_len += sprintf(buffer + proc_len, "\n");
	size_t cnt;
	if (!list_empty(&elevator.list)){
		struct list_head *tmp;
		cnt = 0;
		list_for_each(tmp, &elevator.list)
			cnt++;
	}
	else
	{
		cnt = 0;
	}
	proc_len += sprintf(buffer + proc_len, 
	"Number of passengers: %ld\n", cnt); 
	//if elevator.list has objects, print how many

	size_t waiting = 0;
	for (int i = 0; i < 6; i++){
		struct list_head *pos;
		list_for_each(pos, &floors[i].list)
			if(!(list_empty(&floors[i].list)))
				waiting++;
	}
	proc_len += sprintf(buffer + proc_len, 
	"Number of passengers waiting: %ld\n", waiting); 
	//if fl1.list, fl2.list, etc has objects, add them and print here.

	proc_len += sprintf(buffer + proc_len, 
	"Number of passengers serviced: %d\n", elevator.serviced); 
	//every elev.list.pop (unload), counter++
	
	//free buffer//
	kfree(buffer);

	return simple_read_from_buffer(ubuf, count, ppos, buffer, proc_len);
}

static const struct proc_ops procfile_fops = {
	.proc_read = elevator_read
};


static int __init elevator_init(void) {

	proc_entry = proc_create(ENTRY_NAME, PERMS, PARENT, &procfile_fops);

        // Initialize Elevator passenger list
        INIT_LIST_HEAD(&elevator.list);

        //Initialize floors
        for(int i = 0; i < 6; i++) {
                INIT_LIST_HEAD(&floors[i].list);
        }


        //init elevator
        elevator.floor = 1;
        elevator.wcapacity = 0;
        elevator.pcapacity = 0;
        elevator.state = "OFFLINE";

        stopped = true;
        requesting = false;
	mutex_init(&mutex);

    	STUB_start_elevator = start_elevator;
	STUB_issue_request = issue_request;
	STUB_stop_elevator = stop_elevator;
	if (proc_entry == NULL){
		return -ENOMEM;
	}

	return 0;  // Return 0 to indicate successful loading
}

static void __exit elevator_exit(void) {

   	STUB_start_elevator = NULL;
	STUB_issue_request = NULL;
	STUB_stop_elevator = NULL;

       // Make sure to kfree all linked lists sucha s floors and elvator 

	struct list_head *temp;
        struct list_head *dummy;
        Person * person;

	for(int i = 0; i < 6; i++) {
	        list_for_each_safe(temp, dummy, &floors[i].list)  {
        	        person = list_entry(temp, Person, next);
                	list_del(temp);
                        kfree(person);
		}
	}

	proc_remove(proc_entry);



	mutex_destroy(&mutex);

}

module_init(elevator_init);  // Specify the initialization function
module_exit(elevator_exit);  // Specify the exit/cleanup function
