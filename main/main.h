typedef struct teacher teacher;
typedef struct group group; 

typedef struct student {
    pthread_t thread_id; 
    int student_id; 

    group * assigned_group; 
    teacher *teach; 
    group * tutorial;

    pthread_cond_t * tutorial_assigned; 
    pthread_cond_t * group_assigned; 

    pthread_mutex_t * lock; 
} student; 

typedef struct tutor {
    pthread_t thread_id; 
    int tutor_id; 
    int time;

    teacher * teach;
    group * allocated_group; 

    pthread_cond_t * all_entered; 
    pthread_cond_t * group_assigned; 

    pthread_mutex_t * lock;
} tutor; 

typedef struct group {
    student ** students;
    tutor * tut; 

    int num_students; 
    int attendence; 

    pthread_cond_t * start_tutorial; 
    pthread_cond_t * end_tutorial; 
    pthread_mutex_t * lock; 

    int group_id;
} group; 

struct teacher {
    pthread_t thread_id; 

    student ** students; 
    tutor ** tutors; 
    tutor * vacent_room;

    pthread_cond_t * public_claim_vacency; 
    pthread_cond_t * set_vacent_room; 
    pthread_cond_t * register_entity; 
    pthread_mutex_t * lock; 

    short shutdown_flag; 

    int num_students; 
    int num_groups; 
    int num_tutors; 

    int active_students;
    int active_tutors; 

    int completed_groups; 
};

extern void * destroy_teacher(teacher *);
extern teacher * create_teacher(int, int, int);

extern student ** create_students(int, teacher *);
extern void * destory_students(student **, int);

extern tutor ** create_tutors(int, teacher *, int);
extern void * destroy_tutor(tutor *);

extern group * pop_group(teacher *, group ***);
extern void * destroy_groups(group **, int);
extern group ** create_groups(int);

extern int min(int, int);