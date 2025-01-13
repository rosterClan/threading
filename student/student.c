#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <pthread.h>
#include <main.h>

void * register_student(student * self_student, teacher * teach) {
    pthread_mutex_lock(teach->lock);

    teach->active_students += 1; 
    teach->students = realloc(teach->students,sizeof(student *)*(teach->active_students));
    teach->students[teach->active_students-1] = self_student;

    pthread_cond_signal(teach->register_entity);
    pthread_mutex_unlock(teach->lock);
}

void * group_allocation(student * stud) {
    pthread_mutex_lock(stud->lock);
    printf("Student %i: I've arrived, and I'm waiting to be assigned to a group.\n", stud->student_id);
    while (stud->assigned_group == NULL) {
        pthread_cond_wait(stud->group_assigned, stud->lock);
    }
    printf("Student %i: OK, I'm in group %i. I'm waiting for my turn to enter a lab room.\n", stud->student_id, stud->assigned_group->group_id);   
    pthread_mutex_unlock(stud->lock);
}

void * perform_tutorial(student * stud) {
    group * assigned_group = stud->assigned_group;
    pthread_mutex_lock(assigned_group->lock);
    while (assigned_group->tut == NULL) {
        pthread_cond_wait(assigned_group->start_tutorial, assigned_group->lock);
    }

    printf("Student %i in group %i: My group is called. I will enter the lab room %i now\n",stud->student_id, assigned_group->group_id, assigned_group->tut->tutor_id);
    assigned_group->attendence += 1;
    tutor * tut = stud->assigned_group->tut;
    pthread_cond_signal(assigned_group->tut->all_entered);
    while (assigned_group->tut != NULL) {
        pthread_cond_wait(assigned_group->end_tutorial, assigned_group->lock);
    }
    printf("Student %i in group %i: Thanks Tutor %i. Bye!\n", stud->student_id, assigned_group->group_id, tut->tutor_id);
    pthread_mutex_unlock(assigned_group->lock);
}

void * student_procedure(void * params) {
    student * my_params = (student *)params;

    register_student(my_params, my_params->teach);
    group_allocation(my_params);
    perform_tutorial(my_params);
    
    pthread_exit(params);
}