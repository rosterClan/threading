#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <main.h>
#include <teacher.h>
#include <student.h>
#include <tutor.h>

int min(int val_a, int val_b) {
    if (val_a < val_b) {
        return val_a; 
    }
    return val_b; 
}

group ** create_groups(int n_groups) {
    group ** groups = malloc(sizeof(group*)*n_groups);
    for (int idx = 0; idx < n_groups; idx++) {
        group * new_group = malloc(sizeof(group));
        new_group->group_id = idx; 
        new_group->num_students = 0; 
        new_group->attendence = 0;
        new_group->tut = NULL; 
        new_group->students = malloc(sizeof(void *));

        new_group->lock = malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(new_group->lock, NULL);

        new_group->start_tutorial = malloc(sizeof(pthread_cond_t));
        new_group->end_tutorial = malloc(sizeof(pthread_cond_t));
        pthread_cond_init(new_group->start_tutorial, NULL);
        pthread_cond_init(new_group->end_tutorial, NULL);

        groups[idx] = new_group;
    }
    return groups; 
}

void * destroy_groups(group ** groups, int n_groups) {
    for (int idx = 0; idx < n_groups; idx++) {
        group * curr_group = groups[idx];
        pthread_cond_destroy(curr_group->start_tutorial);
        pthread_cond_destroy(curr_group->end_tutorial);

        free(curr_group->start_tutorial);
        free(curr_group->end_tutorial);

        pthread_mutex_destroy(curr_group->lock);
        free(curr_group->lock);
        free(curr_group->students);

        free(curr_group);
    }
}

group * pop_group(teacher * teach, group *** group_ref) {
    if (teach->num_groups == 0) {
        return NULL; 
    } else if (teach->num_groups == 1) {
        group * subject_group = **(group_ref);
        teach->num_groups--; 
        return subject_group;
    }
    group ** groups = *(group_ref);

    group ** temp = malloc(sizeof(group*)*(teach->num_groups-1));
    for (int idx = 1; idx < teach->num_groups; idx++) {
        temp[idx-1] = groups[idx];
    }
    group * subject_group = groups[0];
    memmove(groups,temp,sizeof(group*)*(teach->num_groups-1));
    free(temp);
    
    teach->num_groups--; 

    return subject_group;
}

tutor ** create_tutors(int number_of_tutors, teacher * teach, int time) {
    tutor ** tutors = malloc(sizeof(tutor)*number_of_tutors);
    for (int idx = 0; idx < number_of_tutors; idx++) {
        tutor * new_tutor = malloc(sizeof(tutor));

        new_tutor->allocated_group = NULL; 
        new_tutor->teach = teach; 
        new_tutor->tutor_id = idx; 

        new_tutor->time = time; 

        new_tutor->all_entered = malloc(sizeof(pthread_cond_t));
        pthread_cond_init(new_tutor->all_entered, NULL);

        new_tutor->group_assigned = malloc(sizeof(pthread_cond_t));
        pthread_cond_init(new_tutor->group_assigned, NULL);

        new_tutor->lock = malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(new_tutor->lock, NULL);

        pthread_create(&new_tutor->thread_id, NULL, tutor_procedure, (void *)new_tutor);
        tutors[idx] = new_tutor;
    }
    return tutors; 
}

void * destroy_tutor(tutor * cur_tutor) {
    pthread_join(cur_tutor->thread_id, NULL);

    pthread_cond_destroy(cur_tutor->all_entered);
    pthread_cond_destroy(cur_tutor->group_assigned);
    pthread_mutex_destroy(cur_tutor->lock);

    free(cur_tutor->all_entered);
    free(cur_tutor->group_assigned);
    free(cur_tutor->lock);
    free(cur_tutor);
}

void * destory_students(student ** students, int num_students) {
    for (int idx = 0; idx < num_students; idx++) {
        student * cur_student = students[idx]; 
        pthread_join(cur_student->thread_id, NULL);

        pthread_cond_destroy(cur_student->tutorial_assigned);
        pthread_cond_destroy(cur_student->group_assigned);
        pthread_mutex_destroy(cur_student->lock);
        
        free(cur_student->tutorial_assigned);
        free(cur_student->group_assigned);
        free(cur_student->lock);
        free(cur_student);
    }
}

student ** create_students(int number_of_students, teacher * teach) {
    student ** students = malloc(sizeof(student *)*number_of_students); 
    for (int idx = 0; idx < number_of_students; idx++) {
        student * new_student = malloc(sizeof(student));

        new_student->assigned_group = NULL;
        new_student->tutorial = NULL; 

        new_student->teach = teach;
        new_student->student_id = idx; 

        new_student->tutorial_assigned = malloc(sizeof(pthread_cond_t));
        pthread_cond_init(new_student->tutorial_assigned, NULL); 

        new_student->group_assigned = malloc(sizeof(pthread_cond_t));
        pthread_cond_init(new_student->group_assigned, NULL); 

        new_student->lock = malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(new_student->lock, NULL);

        pthread_create(&new_student->thread_id, NULL, student_procedure, (void *)new_student);
        students[idx] = new_student;
    }
    return students; 
}

teacher * create_teacher(int num_students, int num_groups, int num_tutors) {
    teacher * teach = malloc(sizeof(teacher));

    teach->completed_groups = 0;
    teach->num_groups = num_groups;
    teach->num_students = num_students;
    teach->num_tutors = num_tutors;
    teach->vacent_room = NULL; 

    teach->register_entity = malloc(sizeof(pthread_cond_t));
    pthread_cond_init(teach->register_entity, NULL); 

    teach->set_vacent_room = malloc(sizeof(pthread_cond_t));
    pthread_cond_init(teach->set_vacent_room, NULL);

    teach->public_claim_vacency = malloc(sizeof(pthread_cond_t));
    pthread_cond_init(teach->public_claim_vacency, NULL);

    teach->lock = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(teach->lock, NULL);

    teach->students = NULL;
    teach->tutors = NULL; 

    pthread_create(&(teach->thread_id), NULL, teacher_procedure, (void *)teach);

    return teach; 
}

void * destroy_teacher(teacher * teach) {
    pthread_join(teach->thread_id, NULL);

    pthread_cond_destroy(teach->register_entity);
    pthread_cond_destroy(teach->set_vacent_room);
    pthread_cond_destroy(teach->public_claim_vacency);
    pthread_mutex_destroy(teach->lock);

    free(teach->register_entity);
    free(teach->set_vacent_room);
    free(teach->public_claim_vacency);
    free(teach->lock);
}

int main()
{
    int number_of_students = 5; 
    int number_of_groups = 5; 
    int number_of_tutors = 1; 
    int time = 1; 
    
    printf("Enter the total number of students (int): ");
    scanf("%d", &number_of_students);
    printf("\n\n"); 
    printf("Enter the total number of groups (int): ");
    scanf("%d", &number_of_groups);
    printf("\n\n"); 
    printf("Enter the total number of tutors (int): ");
    scanf("%d", &number_of_tutors);
    printf("\n\n");
    printf("Enter a unit of time (int): ");
    scanf("%d", &time);

    number_of_groups = min(number_of_groups,number_of_students);
    teacher * teach = create_teacher(number_of_students, number_of_groups, number_of_tutors);
    student ** students = create_students(number_of_students, teach);
    tutor ** tutors = create_tutors(number_of_tutors, teach, time);
    
    free(students);
    free(tutors);

    destroy_teacher(teach);

    return 0;
}