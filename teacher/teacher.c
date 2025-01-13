#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <pthread.h>
#include <main.h>

group ** assign_groups(student ** students, int n_students, int n_groups) {
    printf("Teacher: All students have arrived. I start to assign group ids to students.\n");
    group ** groups = create_groups(n_groups);

    int student_idx = 0;
    int group_idx = 0; 
    while (student_idx < n_students) {
        if (group_idx >= n_groups) {
            group_idx = 0; 
        }

        student * cur_student = *(students+student_idx);
        group * cur_group = groups[group_idx];
        
        pthread_mutex_lock(cur_student->lock);
        cur_student->assigned_group = cur_group; 
        printf("Teacher: student %i is in group %i.\n", cur_student->student_id, cur_group->group_id);

        cur_group->num_students += 1;
        cur_group->students = realloc(cur_group->students, sizeof(student*)*cur_group->num_students);
        cur_group->students[cur_group->num_students-1] = cur_student;
        pthread_cond_signal(cur_student->group_assigned);
        pthread_mutex_unlock(cur_student->lock);

        group_idx += 1; 
        student_idx += 1; 
    }

    return groups;
}

void * run_groups(teacher * teach, group ** groups, tutor ** tutors) {
    pthread_mutex_lock(teach->lock);
    int cur_group_idx = 0; 
    for (int idx = 0; idx < teach->num_groups; idx++) {
        group * subject_group = groups[idx];
        
        while (teach->vacent_room == NULL) {
            pthread_cond_broadcast(teach->public_claim_vacency);
            pthread_cond_wait(teach->set_vacent_room, teach->lock);
        }

        tutor * subject_tutor = teach->vacent_room; 
        teach->vacent_room = NULL; 

        pthread_mutex_lock(subject_tutor->lock);
            subject_tutor->allocated_group = subject_group;
            pthread_cond_signal(subject_tutor->group_assigned);
        pthread_mutex_unlock(subject_tutor->lock);

        printf("Teacher: The lab %i is now available. Students in group %i can enter the room and start your lab exercise.\n",subject_tutor->tutor_id, subject_group->group_id);
    }

    group dummy = {.group_id=-1};
    teach->shutdown_flag = 1; 
    while (teach->num_tutors > 0) {
        while (teach->vacent_room == NULL) {
            pthread_cond_broadcast(teach->public_claim_vacency);
            pthread_cond_wait(teach->set_vacent_room, teach->lock);
        }
        if (teach->num_tutors == 0) {
            break;
        }

        tutor * subject_tutor = teach->vacent_room; 
        teach->vacent_room = NULL; 
        printf("Teacher: There are no students waiting. Tutor %i, you can go home now.\n", subject_tutor->tutor_id);

        pthread_mutex_lock(subject_tutor->lock);
            subject_tutor->allocated_group = &dummy;
            pthread_cond_signal(subject_tutor->group_assigned);
        pthread_mutex_unlock(subject_tutor->lock);

        destroy_tutor(subject_tutor);
        teach->num_tutors--;
    }
}

void * await_threads(teacher * teach) {
    printf("Teacher: I'm waiting for all students to arrive.\n");
    pthread_mutex_lock(teach->lock);
    while (teach->active_students < teach->num_students || teach->active_tutors < teach->num_tutors) {
        pthread_cond_wait(teach->register_entity, teach->lock);
    }
    pthread_mutex_unlock(teach->lock);
}

void * teacher_procedure(void * params) {
    teacher * teach = (teacher *)params;
    await_threads(teach);

    int total_groups = teach->num_groups;
    group ** groups = assign_groups(teach->students, teach->num_students, teach->num_groups);
    group * stack_group_cpy[teach->num_groups];
    memcpy(&stack_group_cpy[0], groups, sizeof(group *)*teach->num_groups);

    run_groups(teach, groups, teach->tutors);

    destroy_groups(&stack_group_cpy[0], total_groups);
    free(groups);

    printf("Teacher: All students and tutors are left. I can now go home.\n");
    pthread_exit(params);
}