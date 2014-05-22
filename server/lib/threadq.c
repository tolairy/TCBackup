#include "../global.h"

#define WORKQ_VALID  0xdec2012
/*
 * Initialize a work queue
 *
 *  Returns: 0 on success
 *           errno on failure
 */
int workq_init(workq_t *wq, int threads, void *(*engine)(void *arg))
{
	int stat;

	if ((stat = pthread_attr_init(&wq->attr)) != 0) {
		return stat;
	}
	if ((stat = pthread_attr_setdetachstate(&wq->attr, PTHREAD_CREATE_DETACHED)) != 0) {
		pthread_attr_destroy(&wq->attr);
		return stat;
	}
	if ((stat = pthread_mutex_init(&wq->mutex, NULL)) != 0) {
		pthread_attr_destroy(&wq->attr);
		return stat;
	}
	if ((stat = pthread_cond_init(&wq->work, NULL)) != 0) {
		pthread_mutex_destroy(&wq->mutex);
		pthread_attr_destroy(&wq->attr);
		return stat;
	}
	wq->quit = 0;
	wq->first = wq->last = NULL;
	wq->max_workers = threads;         /* max threads to create */
	wq->num_workers = 0;               /* no threads yet */
	wq->idle_workers = 0;              /* no idle threads */
	wq->engine = engine;               /* routine to run */
	wq->valid = WORKQ_VALID;           /*flage*/
	return 0;
}

/*
 * Destroy a work queue
 *
 * Returns: 0 on success
 *          errno on failure
 */
int workq_destroy(workq_t *wq)
{
	int stat, stat1, stat2;

	if (wq->valid != WORKQ_VALID) {
		return EINVAL;
	}
	if ((stat = pthread_mutex_lock(&wq->mutex)) != 0) {
		return stat;
	}
	wq->valid = 0;    /* prevent any more operations */

  /*
	* If any threads are active, wake them
  */
	if (wq->num_workers > 0) {
		wq->quit = 1;
		if (wq->idle_workers) {
			if ((stat = pthread_cond_broadcast(&wq->work)) != 0) {
				pthread_mutex_unlock(&wq->mutex);
				return stat;
			}
		}
		while (wq->num_workers > 0) {  /* main thread will be blocked until wq->num_workers=0 */
			
			if ((stat = pthread_cond_wait(&wq->work, &wq->mutex)) != 0) {
				pthread_mutex_unlock(&wq->mutex);
				return stat;
			}
		}
		/*�˴���while��������if��ϸ������*/
	}
	if ((stat = pthread_mutex_unlock(&wq->mutex)) != 0) {
		return stat;
	}
	stat  = pthread_mutex_destroy(&wq->mutex);
	stat1 = pthread_cond_destroy(&wq->work);
	stat2 = pthread_attr_destroy(&wq->attr);

	/*check */
	return (stat != 0 ? stat : (stat1 != 0 ? stat1 : stat2));
}


/*
  �����������ȼ�����priority��0�����ڶ���ͷ�����ڲ��β*/
/*
 *  Add work to a queue
 *    wq is a queue that was created with workq_init
 *    element is a user unique item that will be passed to the
 *        processing routine
 *    work_item will get internal work queue item -- if it is not NULL
 *    priority if non-zero will cause the item to be placed on the
 *        head of the list instead of the tail.
 */
int workq_add(workq_t *wq, void *element, workq_ele_t **work_item, int priority)
{
	int stat;
	workq_ele_t *item;
	pthread_t id;

	//printf("***********workq_add \n");

	
	if (wq->valid != WORKQ_VALID) {
		return EINVAL;
	}

	if ((item = (workq_ele_t *)malloc(sizeof(workq_ele_t))) == NULL) {
		return ENOMEM;
	}
	item->data = element;
	item->next = NULL;

	if ((stat = pthread_mutex_lock(&wq->mutex)) != 0) {
		free(item);
		return stat;
	}

	//printf("***********add item to queue \n");

	
	if (priority) {
		/* Add to head of queue */
		if (wq->first == NULL) {
			wq->first = item;
			wq->last = item;
		} else {
			item->next = wq->first;
			wq->first = item;
		}
	} else {
		/* Add to end of queue */
		if (wq->first == NULL) {
			wq->first = item;
		} else {
			wq->last->next = item;
		}
		wq->last = item;
	}

	/* if any thread is idle, then wake some one*/
	if (wq->idle_workers > 0) {   
		//printf("***********Signal worker \n");
	
	
		if ((stat = pthread_cond_broadcast(&wq->work)) != 0) {
			pthread_mutex_unlock(&wq->mutex);
			return stat;
		}
	} 
	
	else if (wq->num_workers < wq->max_workers) {
         //		printf("***********Create worker thread \n");
		/* No idle threads so create a new one */

		if ((stat = pthread_create(&id, &wq->attr, workq_server, (void *)wq)) != 0) {
			pthread_mutex_unlock(&wq->mutex);
			return stat;
		}
		wq->num_workers++;
	}
	//printf("*****num_workers:%d\n",wq->num_workers);
	
	//printf("*****idel_workers:%d\n",wq->idle_workers);

		pthread_mutex_unlock(&wq->mutex);
	
	/* Return work_item if requested */
	if (work_item) {
		*work_item = item;
	}
	return stat;
}

/*
 *  Remove work from a queue
 *    wq is a queue that was created with workq_init
 *    work_item is an element of work
 *
 *   Note, it is "removed" by immediately calling a processing routine.
 *    if you want to cancel it, you need to provide some external means
 *    of doing so.
 */
int workq_remove(workq_t *wq, workq_ele_t *work_item)
{
	int stat, found = 0;
	pthread_t id;
	workq_ele_t *item, *prev;


	//printf("workq_remove \n");
	
	if (wq->valid != WORKQ_VALID) {
		return EINVAL;
	}

	if ((stat = pthread_mutex_lock(&wq->mutex)) != 0) {
		return stat;
	}
	for (prev=item=wq->first; item; item=item->next) {
		if (item == work_item) {
			found = 1;
			break;
		}
		prev = item;
	}
	if (!found) {
		return EINVAL;
	}

	/* Move item to be first on list */
	if (wq->first != work_item) {
		prev->next = work_item->next;
		if (wq->last == work_item) {
			wq->last = prev;
		}
		work_item->next = wq->first;
		wq->first = work_item;
	}

	/* if any threads are idle, wake one */
	if (wq->idle_workers > 0) {
		if ((stat = pthread_cond_broadcast(&wq->work)) != 0) {
			pthread_mutex_unlock(&wq->mutex);
			return stat;
		}
	}
	
	else if (wq->num_workers < wq->max_workers) 
	
	{
		/* No idle threads so create a new one */
		if ((stat = pthread_create(&id, &wq->attr, workq_server, (void *)wq)) != 0) {
			pthread_mutex_unlock(&wq->mutex);
			return stat;
		}
		wq->num_workers++;
	}
	pthread_mutex_unlock(&wq->mutex);
	return stat;
}


/*
 * This is the worker thread that serves the work queue.
 * In due course, it will call the user's engine.
 */
void *workq_server(void *arg)
{
	struct timespec timeout;
	workq_t *wq = (workq_t *)arg;
	workq_ele_t *we;
	int stat, timedout;
	
	struct timeval tv;
	struct timezone tz;

	 printf("threadid 0x%x is created!\n",pthread_self());
	if ((stat = pthread_mutex_lock(&wq->mutex)) != 0) {
		return NULL;
	}
	
     for (;;) 
	
	{
		
		timedout = 0;
		gettimeofday(&tv, &tz);
		timeout.tv_nsec = 0;
		timeout.tv_sec = tv.tv_sec + 5;
		//printf("-----------num_workers:%d\n",wq->num_workers);
		
		//printf("-----------idel_workers:%d\n",wq->idle_workers);
		
		while (wq->first == NULL && !wq->quit) 
		
		{
			/*Wait 2 seconds, then if no more work, exit*/

			//printf("********pthread_cond_timedwait()\n");
			
			wq->idle_workers++;
			stat = pthread_cond_timedwait(&wq->work, &wq->mutex, &timeout); 
			wq->idle_workers--;


			//printf("********timedwait=%d\n",stat);

		// 	printf("********ETIMEDOUt=%d\n",ETIMEDOUT);
			
			if (stat == ETIMEDOUT) {
				timedout = 1;
				break;
			} else if (stat != 0) {
				/* This shouldn't happen */
				//printf("This shouldn't happen\n");
				wq->num_workers--;
				pthread_mutex_unlock(&wq->mutex);
				return NULL;
			}
		}
		we = wq->first; /*get a socket thread to execute from the queue */
		if (we != NULL) 
        {
			wq->first = we->next;
			if (wq->last == we) {
				wq->last = NULL;
			}
			if ((stat = pthread_mutex_unlock(&wq->mutex)) != 0) {
				return NULL;
			}
			/* Call user's routine here */

			wq->engine(we->data);

			free(we);    /* release work entry */

			if ((stat = pthread_mutex_lock(&wq->mutex)) != 0) {
				return NULL;
			}

		}
      		/*
		* If no more work request, and we are asked to quit, then do it
		*/
    

		if (wq->first == NULL && wq->quit) {
			wq->num_workers--;
			if (wq->num_workers == 0) {

				//printf("Wake up destroy routine\n");
				/* Wake up destroy routine if he is waiting */
				pthread_cond_broadcast(&wq->work);
			}

			pthread_mutex_unlock(&wq->mutex);

			printf("threadid 0x%x is exit normally!\n",pthread_self());
					return NULL;
		}
	
      		/*
		* If no more work requests, and we waited long enough, quit
     		 */

		if (wq->first == NULL && timedout) {

			printf("timedout,exit ,threadid 0x%x\n",pthread_self());
			wq->num_workers--;
			break;
		}
		
	} /* end of big for loop */

	pthread_mutex_unlock(&wq->mutex);
	return NULL;
}



//#define THREAD_TEST

#ifdef THREAD_TEST
void * myprocess (void *arg) 
{ 
	printf ("threadid is 0x%x, working on task %d\n", pthread_self (),*(int *) arg); 
	sleep (3);
	return NULL; 
} 


int 
main (int argc, char *argv[])
{
	workq_t wq;
	
	workq_ele_t *work_item;

	int *workingnum ;
	
	int i; 
	workingnum = (int *) malloc (sizeof (int) * 10);
	workq_init(&wq, 3, myprocess);
	for (i = 0; i < 10; i++) 
	{ 
		workingnum[i] = i;
		
		workq_add(&wq, &workingnum[i], &work_item, 0);
		
		if(i==9)
			
		workq_remove(&wq, work_item);
		
	
	} 
   
	sleep (60); 
	if( workq_destroy(&wq)!=0)
		
		printf("destroy error:\n");
	
	free (workingnum); 
	return 0; 

}
#endif
