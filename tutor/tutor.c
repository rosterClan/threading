#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <pthread.h>
#include <main.h>

void * register_tutor(tutor * self_tutor, teacher * teach) {
    pthread_mutex_lock(teach->lock);

    teach->active_tutors += 1; 
    teach->tutors = realloc(teach->tutors,sizeof(tutor *)*(teach->active_tutors));
    teach->tutors[teach->active_tutors-1] = self_tutor;

    pthread_cond_signal(teach->register_entity);
    pthread_mutex_unlock(teach->lock);
}

void * run_tutorial(tutor * tut) {
    group * cur_group = tut->allocated_group;
    
    pthread_mutex_lock(cur_group->lock);
    tut->allocated_group->tut = tut; 
    pthread_cond_broadcast(cur_group->start_tutorial);
    while (cur_group->attendence < cur_group->num_students) {
        pthread_cond_wait(tut->all_entered, cur_group->lock);
    }

    printf("Tutor %i: All students in group %i have entered the room %i. You can start your exercise now.\n", tut->tutor_id, cur_group->group_id, tut->tutor_id);
    int time = rand() % (tut->time + 1 - tut->time) + tut->time;
    sleep(time);
    printf("Tutor %i: Students in group %i have completed the lab exercise in %i units of time. You may leave this room now.\n", tut->tutor_id, cur_group->group_id, tut->tutor_id);
    
    cur_group->tut = NULL; 
    pthread_cond_broadcast(cur_group->end_tutorial);
    pthread_mutex_unlock(cur_group->lock);

    destory_students(cur_group->students, cur_group->attendence);
}

void * call_for_group(tutor * tut) {
    while (true) {
        pthread_mutex_lock(tut->teach->lock); // We need to set the vacent room
            while (tut->teach->vacent_room != NULL) {
                pthread_cond_wait(tut->teach->public_claim_vacency,tut->teach->lock);
            }
            tut->teach->vacent_room = tut; 
            pthread_cond_signal(tut->teach->set_vacent_room);
        pthread_mutex_unlock(tut->teach->lock);

        pthread_mutex_lock(tut->lock); // We need to wait for a group to be assigned to us
        while (tut->allocated_group == NULL) {
            pthread_cond_wait(tut->group_assigned, tut->lock);
        }

        if (tut->teach->shutdown_flag && tut->allocated_group->group_id == -1) {
            tut->allocated_group = NULL; 
            pthread_mutex_unlock(tut->lock);
            break; 
        } else {
            run_tutorial(tut);
            tut->allocated_group = NULL; 
            pthread_mutex_unlock(tut->lock);
        }
    }
    
    printf("Tutor %i: Thanks Teacher. Bye!\n", tut->tutor_id);
    pthread_cond_signal(tut->teach->set_vacent_room);
}

void * tutor_procedure(void * params) {
    tutor * my_params = (tutor *)params;
    register_tutor(my_params, my_params->teach);
    call_for_group(my_params);

    pthread_exit(params);
}